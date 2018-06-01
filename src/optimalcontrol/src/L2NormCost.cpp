/*
 * Copyright (C) 2014,2018 Fondazione Istituto Italiano di Tecnologia
 *
 * Licensed under either the GNU Lesser General Public License v3.0 :
 * https://www.gnu.org/licenses/lgpl-3.0.html
 * or the GNU Lesser General Public License v2.1 :
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 * at your option.
 *
 * Originally developed for Prioritized Optimal Control (2014)
 * Refactored in 2018.
 * Design inspired by
 * - ACADO toolbox (http://acado.github.io)
 * - ADRL Control Toolbox (https://adrlab.bitbucket.io/ct/ct_doc/doc/html/index.html)
 */

#include <iDynTree/L2NormCost.h>
#include <iDynTree/Core/MatrixDynSize.h>
#include <iDynTree/Core/VectorDynSize.h>
#include <iDynTree/Core/Utils.h>
#include <iDynTree/Core/TestUtils.h>
#include <cassert>

#include <Eigen/Dense>
#include <iDynTree/Core/EigenHelpers.h>

namespace iDynTree {
    namespace optimalcontrol {

    class TimeVaryingGradient : public TimeVaryingVector {
        MatrixDynSize m_selectorMatrix, m_weightMatrix;
        MatrixDynSize m_subMatrix; //weightMatrix times selector
        std::shared_ptr<TimeVaryingVector> m_desiredTrajectory;
        VectorDynSize m_outputVector;
    public:
        TimeVaryingGradient(const MatrixDynSize& selectorMatrix)
        : m_selectorMatrix(selectorMatrix)
        , m_desiredTrajectory(nullptr)
        {
            m_weightMatrix.resize(m_selectorMatrix.rows(), m_selectorMatrix.rows());
            toEigen(m_weightMatrix).setIdentity();
            m_subMatrix = /*m_pimpl->stateWeight * */ m_selectorMatrix;
            m_outputVector.resize(m_selectorMatrix.cols());
            m_outputVector.zero();
        }

        ~TimeVaryingGradient() override;

        bool setDesiredTrajectory(std::shared_ptr<TimeVaryingVector> desiredTrajectory) {
            if (!desiredTrajectory) {
                reportError("TimeVaryingGradient", "desiredTrajectory", "Empty desired trajectory pointer.");
                return false;
            }
            m_desiredTrajectory = desiredTrajectory;
            return true;
        }

        bool setWeightMatrix(const MatrixDynSize &weights)
        {
            if (weights.rows() != weights.cols()) {
                reportError("TimeVaryingGradient", "setWeightMatrix", "The weights matrix is supposed to be squared.");
                return false;
            }

            if (weights.cols() != m_selectorMatrix.rows()) {
                reportError("TimeVaryingGradient", "setWeightMatrix", "The weights matrix dimensions do not match those of the specified selector.");
                return false;
            }

            m_weightMatrix = weights;
            toEigen(m_subMatrix) = toEigen(m_weightMatrix) * toEigen(m_selectorMatrix);

            return true;
        }

        virtual const VectorDynSize& getObject(double time, bool &isValid) override {
            if (!m_desiredTrajectory) {
                isValid = true;
                return m_outputVector; //should be zero from the initialization
            }
            bool ok = false;
            const VectorDynSize &desiredPoint = m_desiredTrajectory->getObject(time, ok);

            if (!ok) {
                isValid = false;
                m_outputVector.zero();
                return m_outputVector;
            }

            if (desiredPoint.size() != m_subMatrix.rows()) {
                std::ostringstream errorMsg;
                errorMsg << "The specified desired point at time: " << time << " has size not matching the specified selector.";
                reportError("TimeVaryingGradient", "getObject", errorMsg.str().c_str());
                isValid = false;
                m_outputVector.zero();
                return m_outputVector;
            }

            toEigen(m_outputVector) = -1.0 * toEigen(desiredPoint).transpose() * toEigen(m_subMatrix);

            isValid = true;
            return m_outputVector;
        }

        const MatrixDynSize& selector() {
            return m_selectorMatrix;
        }

        const MatrixDynSize& subMatrix() {
            return m_subMatrix;
        }
    };
    TimeVaryingGradient::~TimeVaryingGradient() {};


        class L2NormCost::L2NormCostImplementation {
            public:
            std::shared_ptr<TimeVaryingGradient> stateGradient, controlGradient;
            std::shared_ptr<TimeInvariantMatrix> stateHessian, controlHessian;

            void initialize(const MatrixDynSize &stateSelector, const MatrixDynSize &controlSelector) {
                if ((stateSelector.rows() != 0) && (stateSelector.cols() != 0)) {
                    stateGradient = std::make_shared<TimeVaryingGradient>(stateSelector);
                    stateHessian = std::make_shared<TimeInvariantMatrix>();
                    stateHessian->get().resize(stateSelector.cols(), stateSelector.cols());
                    toEigen(stateHessian->get()) = toEigen(stateGradient->selector()).transpose() * toEigen(stateGradient->subMatrix());
                } else {
                    stateGradient = nullptr;
                }

                if ((controlSelector.rows() != 0) && (controlSelector.cols() != 0)) {
                    controlGradient = std::make_shared<TimeVaryingGradient>(controlSelector);
                    controlHessian = std::make_shared<TimeInvariantMatrix>();
                    controlHessian->get().resize(controlSelector.cols(), controlSelector.cols());
                    toEigen(controlHessian->get()) = toEigen(controlGradient->selector()).transpose() * toEigen(controlGradient->subMatrix());
                } else {
                    controlGradient = nullptr;
                }
            }

            void initialize(unsigned int stateDimension, unsigned int controlDimension) {
                MatrixDynSize stateSelector(stateDimension, stateDimension), controlSelector(controlDimension, controlDimension);
                toEigen(stateSelector).setIdentity();
                toEigen(controlSelector).setIdentity();
                initialize(stateSelector, controlSelector);
            }

        };


        L2NormCost::L2NormCost(const std::string &name, unsigned int stateDimension, unsigned int controlDimension)
        : QuadraticCost(name)
        , m_pimpl(new L2NormCostImplementation)
        {
            assert(m_pimpl);

            m_pimpl->initialize(stateDimension, controlDimension);

            if (m_pimpl->stateGradient) {
                bool ok;
                ok = setStateCost(m_pimpl->stateHessian, m_pimpl->stateGradient);
                ASSERT_IS_TRUE(ok);
            }

            if (m_pimpl->controlGradient) {
                bool ok;
                ok = setControlCost(m_pimpl->controlHessian, m_pimpl->controlGradient);
                ASSERT_IS_TRUE(ok);
            }
        }

        L2NormCost::L2NormCost(const std::string &name, const MatrixDynSize &stateSelector, const MatrixDynSize &controlSelector)
        : QuadraticCost(name)
        , m_pimpl(new L2NormCostImplementation)
        {
            m_pimpl->initialize(stateSelector, controlSelector);

            if (m_pimpl->stateGradient) {
                bool ok;
                ok = setStateCost(m_pimpl->stateHessian, m_pimpl->stateGradient);
                ASSERT_IS_TRUE(ok);
            }

            if (m_pimpl->controlGradient) {
                bool ok;
                ok = setControlCost(m_pimpl->controlHessian, m_pimpl->controlGradient);
                ASSERT_IS_TRUE(ok);
            }
        }

        L2NormCost::~L2NormCost()
        {
            if (m_pimpl){
                delete m_pimpl;
                m_pimpl = nullptr;
            }
        }

        bool L2NormCost::setStateWeight(const MatrixDynSize &stateWeights)
        {
            if (stateWeights.rows() != stateWeights.cols()) {
                reportError("L2NormCost", "setStateWeight", "The stateWeights matrix is supposed to be squared.");
                return false;
            }

            if (!(m_pimpl->stateGradient)) {
                reportError("L2NormCost", "setStateWeight", "The state cost portion has been deactivated, given the provided selectors.");
                return false;
            }

            if (!(m_pimpl->stateGradient->setWeightMatrix(stateWeights))) {
                reportError("L2NormCost", "setStateWeight", "Error when specifying the state weights.");
                return false;
            }

            return true;
        }

        bool L2NormCost::setStateDesiredPoint(const VectorDynSize &desiredPoint)
        {
            if (!(m_pimpl->stateGradient)) {
                reportError("L2NormCost", "setStateDesiredPoint", "The state cost portion has been deactivated, given the provided selectors.");
                return false;
            }

            if (desiredPoint.size() != (m_pimpl->stateGradient->subMatrix().rows())) {
                reportError("L2NormCost", "setStateDesiredPoint", "The desiredPoint size do not match the dimension of the specified selector.");
                return false;
            }

            std::shared_ptr<TimeInvariantVector> newTrajectory = std::make_shared<TimeInvariantVector>(desiredPoint);
            return m_pimpl->stateGradient->setDesiredTrajectory(newTrajectory);
        }

        bool L2NormCost::setStateDesiredTrajectory(std::shared_ptr<TimeVaryingVector> stateDesiredTrajectory)
        {
            if (!(m_pimpl->stateGradient)) {
                reportError("L2NormCost", "setStateDesiredTrajectory", "The state cost portion has been deactivated, given the provided selectors.");
                return false;
            }

            if (!stateDesiredTrajectory) {
                reportError("L2NormCost", "setStateDesiredTrajectory", "Empty desired trajectory pointer.");
                return false;
            }

            return m_pimpl->stateGradient->setDesiredTrajectory(stateDesiredTrajectory);
        }

        bool L2NormCost::setControlWeight(const MatrixDynSize &controlWeights)
        {
            if (controlWeights.rows() != controlWeights.cols()) {
                reportError("L2NormCost", "setControlWeight", "The controlWeights matrix is supposed to be squared.");
                return false;
            }

            if (!(m_pimpl->controlGradient)) {
                reportError("L2NormCost", "setControlWeight", "The control cost portion has been deactivated, given the provided selectors.");
                return false;
            }

            if (!(m_pimpl->controlGradient->setWeightMatrix(controlWeights))) {
                reportError("L2NormCost", "setControlWeight", "Error when specifying the control weights.");
                return false;
            }

            return true;
        }

        bool L2NormCost::setControlDesiredPoint(const VectorDynSize &desiredPoint)
        {
            if (!(m_pimpl->controlGradient)) {
                reportError("L2NormCost", "setControlDesiredPoint", "The control cost portion has been deactivated, given the provided selectors.");
                return false;
            }

            if (desiredPoint.size() != (m_pimpl->controlGradient->subMatrix().rows())) {
                reportError("L2NormCost", "setControlDesiredPoint", "The desiredPoint size do not match the dimension of the specified selector.");
                return false;
            }

            std::shared_ptr<TimeInvariantVector> newTrajectory = std::make_shared<TimeInvariantVector>(desiredPoint);
            return m_pimpl->controlGradient->setDesiredTrajectory(newTrajectory);
        }

        bool L2NormCost::setControlDesiredTrajectory(std::shared_ptr<TimeVaryingVector> controlDesiredTrajectory)
        {
            if (!(m_pimpl->controlGradient)) {
                reportError("L2NormCost", "setControlDesiredTrajectory", "The control cost portion has been deactivated, given the provided selectors.");
                return false;
            }

            if (!controlDesiredTrajectory) {
                reportError("L2NormCost", "setControlDesiredTrajectory", "Empty desired trajectory pointer.");
                return false;
            }

            return m_pimpl->controlGradient->setDesiredTrajectory(controlDesiredTrajectory);
        }

    }
}


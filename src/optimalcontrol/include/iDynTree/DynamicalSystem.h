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

#ifndef IDYNTREE_OPTIMALCONTROL_DYNAMICALSYSTEM_H
#define IDYNTREE_OPTIMALCONTROL_DYNAMICALSYSTEM_H

#include <iDynTree/Core/VectorDynSize.h>

namespace iDynTree {

    class MatrixDynSize;

namespace optimalcontrol {

    /**
     * @warning This class is still in active development, and so API interface can change between iDynTree versions.
     * \ingroup iDynTreeExperimental
     */

    /**
     * @brief DynamicalSystem base class.
     *
     * It defines a continuos time dynamical system, i.e. \f$ \dot{x} = f(t,x,u) \f$
     * Inherit publicly from this class in order to define your custiom dynamical system.
     */

    class DynamicalSystem {

    public:

        DynamicalSystem() = delete;

        /**
         * @brief Default constructor
         *
         * @param[in] stateSpaceSize Dimension of the state space.
         * @param[in] controlSpaceSize Dimension of the control space.
         */
        DynamicalSystem(size_t stateSpaceSize,
                        size_t controlSpaceSize);

        DynamicalSystem(const DynamicalSystem& other) = delete;

        virtual ~DynamicalSystem();

        /**
         * @brief Returns the state space dimension.
         */
        size_t stateSpaceSize() const;

        /**
         * @brief Returns the control space dimension.
         */
        size_t controlSpaceSize() const;

        /**
         * @brief Computes the system dynamics.
         *
         * It return \f$ f(t,x) \f$. Notice that here the dependency from the control input is removed, so that basically we are assuming an autonomous system.
         * If the system is controlled, the control input will be set separately with the method setControlInput. This was necessary since the Integrator class needs
         * an autonomous system to be integrated. See ControlledDynamicalSystem class in case you want to join a DynamicalSystem with a Controller.
         * @param[in] state The state point in which the dynamics is computed.
         * @param[in] time  The time at which the dynamics is computed.
         * @param[out] stateDynamics The value of the state derivative.
         * @return True if successfull.
         */
        virtual bool dynamics(const VectorDynSize& state,
                              double time,
                              VectorDynSize& stateDynamics) = 0;

        /**
         * @brief Set the control input to the dynamical system.
         *
         * In principle, there is no need to override this method.
         * This value is stored in an internal buffer which can be accessed through the method controlInput().
         * @param[in] control The control input value.
         * @return True if successful, false otherwise (for example if size do not match).
         */
        virtual bool setControlInput(const VectorDynSize &control);

        /**
         * @brief Access the control input.
         *
         * In principle, there is no need to override this method.
         * This has to be set with the method setControlInput().
         * @return Const reference to the control input buffer.
         */
        virtual const VectorDynSize& controlInput() const;

        /**
         * @brief Access the control input.
         *
         * In principle, there is no need to override this method.
         * This has to be set with the method setControlInput().
         * @param[in] index Index at which accessing the control input buffer.
         * @return Value corresponding to the specified index.
         */
        virtual double controlInput(unsigned int index) const;

        /**
         * @brief Access the initial state.
         *
         * In principle, there is no need to override this method.
         * This has to be set with the method setInitialState().
         * @return Const reference to the initial state buffer.
         */
        virtual const VectorDynSize& initialState() const;

        /**
         * @brief Access the initial state.
         *
         * In principle, there is no need to override this method.
         * This has to be set with the method setInitialState().
         * @param[in] index Index at which accessing the initial state buffer.
         * @return Value corresponding to the specified index.
         */
        virtual double initialState(unsigned int index) const;

        /**
         * @brief Set the initial state to the dynamical system.
         *
         * In principle, there is no need to override this method.
         * This value is stored in an internal buffer which can be accessed through the method initialState().
         * @param[in] state The initial state value.
         * @return True if successful, false otherwise (for example if size do not match).
         */
        virtual bool setInitialState(const VectorDynSize &state);

        /**
         * @brief Compute the partial derivative of the state dynamics wrt the state.
         *
         * Namely it computes, \f$ \frac{\partial f(t,x,u)}{\partial x}\f$.
         * By default it return false;
         *
         * @param[in] state The state value at which computing the partial derivative.
         * @param[in] time The time at which computing the partial derivative.
         * @param[out] dynamicsDerivative The output derivative. It has to be a square matrix with dimension equal to the state size.
         * @return True if successful, false otherwise (or if not implemented).
         */
        virtual bool dynamicsStateFirstDerivative(const VectorDynSize& state,
                                                  double time,
                                                  MatrixDynSize& dynamicsDerivative);

        /**
         * @brief Compute the partial derivative of the state dynamics wrt the control.
         *
         * Namely it computes, \f$ \frac{\partial f(t,x,u)}{\partial u}\f$.
         * By default it return false;
         *
         * @param[in] state The state value at which computing the partial derivative.
         * @param[in] time The time at which computing the partial derivative.
         * @param[out] dynamicsDerivative The output derivative. It has to be a matrix with number of rows equal to the state size and number of columns equal to the control size.
         * @return True if successful, false otherwise (or if not implemented).
         */
        virtual bool dynamicsControlFirstDerivative(const VectorDynSize& state,
                                                    double time,
                                                    MatrixDynSize& dynamicsDerivative);


        /**
         * @brief Returns the set of nonzeros elements in terms of row and colun index, in the state jacobian
         * @param nonZeroElementRows The row indeces of non-zero elements. The corresponding columns are in the nonZeroElementColumns vector.
         * @param nonZeroElementColumns The column indeces of non-zero elements. The corresponding rows are in the nonZeroElementRows vector.
         * @return true if the sparsity is available. False otherwise.
         */
        virtual bool dynamicsStateFirstDerivativeSparsity(std::vector<size_t>& nonZeroElementRows, std::vector<size_t>& nonZeroElementColumns);

        /**
         * @brief Returns the set of nonzeros elements in terms of row and colun index, in the control jacobian
         * @param nonZeroElementRows The row indeces of non-zero elements. The corresponding columns are in the nonZeroElementColumns vector.
         * @param nonZeroElementColumns The column indeces of non-zero elements. The corresponding rows are in the nonZeroElementRows vector.
         * @return true if the sparsity is available. False otherwise.
         */
        virtual bool dynamicsControlFirstDerivativeSparsity(std::vector<size_t>& nonZeroElementRows, std::vector<size_t>& nonZeroElementColumns);

    private:
        size_t m_stateSize;
        size_t m_controlSize;
        VectorDynSize m_initialState;
        VectorDynSize m_controlInput;
    };
    
}
}



#endif /* end of include guard: IDYNTREE_OPTIMALCONTROL_DYNAMICALSYSTEM_H */

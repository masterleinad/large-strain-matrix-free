#pragma once

#include <deal.II/base/exceptions.h>
#include <deal.II/base/vectorization.h>
#include <deal.II/base/subscriptor.h>

#include <deal.II/lac/diagonal_matrix.h>
#include <deal.II/lac/la_parallel_vector.h>
#include <deal.II/lac/vector.h>
#include <deal.II/multigrid/mg_constrained_dofs.h>
#include <deal.II/matrix_free/matrix_free.h>
#include <deal.II/matrix_free/fe_evaluation.h>

#include <material.h>

using namespace dealii;

  /**
   * Large strain Neo-Hook tangent operator.
   *
   * Follow https://github.com/dealii/dealii/blob/master/tests/matrix_free/step-37.cc
   */
  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  class NeoHookOperator : public Subscriptor
  {
  public:
    NeoHookOperator ();

    typedef typename Vector<number>::size_type size_type;

    void clear();

    void initialize(std::shared_ptr<const MatrixFree<dim,number>> data_current,
                    std::shared_ptr<const MatrixFree<dim,number>> data_reference,
                    Vector<number> &displacement);

    void set_material(std::shared_ptr<Material_Compressible_Neo_Hook_One_Field<dim,VectorizedArray<number>>> material);

    void compute_diagonal();

    unsigned int m () const;
    unsigned int n () const;

    void vmult (Vector<double> &dst,
                const Vector<double> &src) const;

    void Tvmult (Vector<double> &dst,
                 const Vector<double> &src) const;
    void vmult_add (Vector<double> &dst,
                    const Vector<double> &src) const;
    void Tvmult_add (Vector<double> &dst,
                     const Vector<double> &src) const;

    number el (const unsigned int row,
               const unsigned int col) const;

    void precondition_Jacobi(Vector<number> &dst,
                             const Vector<number> &src,
                             const number omega) const;

  private:

    /**
     * Apply operator on a range of cells.
     */
    void local_apply_cell (const MatrixFree<dim,number>    &data,
                           Vector<double>                      &dst,
                           const Vector<double>                &src,
                           const std::pair<unsigned int,unsigned int> &cell_range) const;

    /**
     * Apply diagonal part of the operator on a cell range.
     */
    void local_diagonal_cell (const MatrixFree<dim,number> &data,
                              Vector<double>                                   &dst,
                              const unsigned int &,
                              const std::pair<unsigned int,unsigned int>       &cell_range) const;


   /**
    * Perform operation on a cell. @p phi_current and @phi_current_s correspond to the deformed configuration
    * where @p phi_reference is for the current configuration.
    */
   void do_operation_on_cell(FEEvaluation<dim,fe_degree,n_q_points_1d,dim,number> &phi_current,
                             FEEvaluation<dim,fe_degree,n_q_points_1d,dim,number> &phi_current_s,
                             FEEvaluation<dim,fe_degree,n_q_points_1d,dim,number> &phi_reference,
                             const unsigned int cell) const;

    std::shared_ptr<const MatrixFree<dim,number>> data_current;
    std::shared_ptr<const MatrixFree<dim,number>> data_reference;

    Vector<number> *displacement;

    std::shared_ptr<Material_Compressible_Neo_Hook_One_Field<dim,VectorizedArray<number>>> material;

    std::shared_ptr<DiagonalMatrix<Vector<number>>>  inverse_diagonal_entries;
    std::shared_ptr<DiagonalMatrix<Vector<number>>>  diagonal_entries;

    bool            diagonal_is_available;
  };



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::NeoHookOperator ()
    :
    Subscriptor(),
    diagonal_is_available(false)
  {}



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  void
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::precondition_Jacobi(Vector<number> &dst,
                                            const Vector<number> &src,
                                            const number omega) const
  {
    Assert(inverse_diagonal_entries.get() &&
           inverse_diagonal_entries->m() > 0, ExcNotInitialized());
    inverse_diagonal_entries->vmult(dst,src);
    dst *= omega;
  }



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  unsigned int
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::m () const
  {
    return data_current.get_vector_partitioner()->size();
  }



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  unsigned int
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::n () const
  {
    return data_current.get_vector_partitioner()->size();
  }



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  void
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::clear ()
  {
    data_current.reset();
    data_reference.reset();
    diagonal_is_available = false;
    diagonal_entries.reset();
    inverse_diagonal_entries.reset();
  }



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  void
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::initialize(
                    std::shared_ptr<const MatrixFree<dim,number>> data_current_,
                    std::shared_ptr<const MatrixFree<dim,number>> data_reference_,
                    Vector<number> &displacement_)
  {
    data_current = data_current_;
    data_reference = data_reference_;
    displacement = &displacement_;
  }



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  void
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::set_material(std::shared_ptr<Material_Compressible_Neo_Hook_One_Field<dim,VectorizedArray<number>>> material_)
  {
    material = material_;
  }



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  void
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::vmult (Vector<double>       &dst,
                                                const Vector<double> &src) const
  {
    dst = 0;
    vmult_add (dst, src);
  }



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  void
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::Tvmult (Vector<double>       &dst,
                                                 const Vector<double> &src) const
  {
    dst = 0;
    vmult_add (dst,src);
  }



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  void
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::Tvmult_add (Vector<double>       &dst,
                                                     const Vector<double> &src) const
  {
    vmult_add (dst,src);
  }



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  void
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::vmult_add (Vector<double>       &dst,
                                                    const Vector<double> &src) const
  {
    // FIXME: can't use cell_loop as we need both matrix-free data objects.
    // for now do it by hand.
    // BUT I might try cell_loop(), and simply use another MF object inside...

    Assert (data_current->n_macro_cells() == data_reference->n_macro_cells(), ExcInternalError());

    // MatrixFree::cell_loop() is more complicated than a simple update_ghost_values() / compress(),
    // it loops on different cells (inner without ghosts and outer) in different order
    // and do update_ghost_values() and compress_start()/compress_finish() in between.
    // https://www.dealii.org/developer/doxygen/deal.II/matrix__free_8h_source.html#l00109

    // 1. make sure ghosts are updated
    // src.update_ghost_values();

    // 2. loop over all locally owned cell blocks
    local_apply_cell(*data_current, dst, src,
                     std::make_pair<unsigned int,unsigned int>(0,data_current->n_macro_cells()));

    // 3. communicate results with MPI
    // dst.compress(VectorOperation::add);

    // 4. constraints
    const std::vector<unsigned int> &
    constrained_dofs = data_current->get_constrained_dofs(); // FIXME: is it current or reference?
    for (unsigned int i=0; i<constrained_dofs.size(); ++i)
      dst(constrained_dofs[i]) += src(constrained_dofs[i]);
  }



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  void
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::local_apply_cell (
                           const MatrixFree<dim,number>    &/*data*/,
                           Vector<double>                      &dst,
                           const Vector<double>                &src,
                           const std::pair<unsigned int,unsigned int> &cell_range) const
  {
    // FIXME: I don't use data input, can this be bad?

    FEEvaluation<dim,fe_degree,n_q_points_1d,dim,number> phi_current  (*data_current);
    FEEvaluation<dim,fe_degree,n_q_points_1d,dim,number> phi_current_s(*data_current);
    FEEvaluation<dim,fe_degree,n_q_points_1d,dim,number> phi_reference(*data_reference);

    Assert (phi_current.n_q_points == phi_reference.n_q_points, ExcInternalError());

    for (unsigned int cell=cell_range.first; cell<cell_range.second; ++cell)
      {
        // initialize on this cell
        phi_current.reinit(cell);
        phi_current_s.reinit(cell);
        phi_reference.reinit(cell);

        // read-in total displacement and src vector and evaluate gradients
        phi_reference.read_dof_values_plain(*displacement);
        phi_current.  read_dof_values(src);
        phi_current_s.read_dof_values(src);

        do_operation_on_cell(phi_current,phi_current_s,phi_reference,cell);

        phi_current.distribute_local_to_global(dst);
        phi_current_s.distribute_local_to_global(dst);
      }
  }



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  void
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::local_diagonal_cell (const MatrixFree<dim,number> &/*data*/,
                              Vector<double>                                   &dst,
                              const unsigned int &,
                              const std::pair<unsigned int,unsigned int>       &cell_range) const
  {
    // FIXME: I don't use data input, can this be bad?

    FEEvaluation<dim,fe_degree,n_q_points_1d,dim,number> phi_current  (*data_current);
    FEEvaluation<dim,fe_degree,n_q_points_1d,dim,number> phi_current_s(*data_current);
    FEEvaluation<dim,fe_degree,n_q_points_1d,dim,number> phi_reference(*data_reference);

    for (unsigned int cell=cell_range.first; cell<cell_range.second; ++cell)
      {
        // initialize on this cell
        phi_current.reinit(cell);
        phi_current_s.reinit(cell);
        phi_reference.reinit(cell);

        // read-in total displacement.
        phi_reference.read_dof_values_plain(*displacement);

        // FIXME: although we override DoFs manually later, somehow
        // we still need to read some dummy here
        phi_current.read_dof_values(*displacement);
        phi_current_s.read_dof_values(*displacement);

        AlignedVector<VectorizedArray<number>> local_diagonal_vector(phi_current.dofs_per_component*phi_current.n_components);

        // Loop over all DoFs and set dof values to zero everywhere but i-th DoF.
        // With this input (instead of read_dof_values()) we do the action and store the
        // result in a diagonal vector
        for (unsigned int i=0; i<phi_current.dofs_per_component; ++i)
          for (unsigned int ic=0; ic<phi_current.n_components; ++ic)
            {
              for (unsigned int j=0; j<phi_current.dofs_per_component; ++j)
                for (unsigned int jc=0; jc<phi_current.n_components; ++jc)
                  {
                    const auto ind_j = j+jc*phi_current.dofs_per_component;
                    phi_current.begin_dof_values()  [ind_j] = VectorizedArray<number>();
                    phi_current_s.begin_dof_values()[ind_j] = VectorizedArray<number>();
                  }

              const auto ind_i = i+ic*phi_current.dofs_per_component;

              phi_current.begin_dof_values()  [ind_i] = 1.;
              phi_current_s.begin_dof_values()[ind_i] = 1.;

              do_operation_on_cell(phi_current,phi_current_s,phi_reference,cell);

              local_diagonal_vector[ind_i] = phi_current.begin_dof_values()[ind_i] +
                                             phi_current_s.begin_dof_values()[ind_i];
            }

        // Finally, in order to distribute diagonal, write it again into one of
        // FEEvaluations and do the standard distribute_local_to_global.
        // Note that here non-diagonal matrix elements are ignored and so the result is
        // not equivalent to matrix-based case when hanging nodes are present.
        // see Section 5.3 in Korman 2016, A time-space adaptive method for the Schrodinger equation, doi: 10.4208/cicp.101214.021015a
        // for a discussion.
        for (unsigned int i=0; i<phi_current.dofs_per_component; ++i)
          for (unsigned int ic=0; ic<phi_current.n_components; ++ic)
            {
              const auto ind_i = i+ic*phi_current.dofs_per_component;
              phi_current.begin_dof_values()[ind_i] = local_diagonal_vector[ind_i];
            }

        phi_current.distribute_local_to_global (dst);
      } // end of cell loop
  }



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  void
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::do_operation_on_cell(
                             FEEvaluation<dim,fe_degree,n_q_points_1d,dim,number> &phi_current,
                             FEEvaluation<dim,fe_degree,n_q_points_1d,dim,number> &phi_current_s,
                             FEEvaluation<dim,fe_degree,n_q_points_1d,dim,number> &phi_reference,
                             const unsigned int /*cell*/) const
  {
    phi_reference.evaluate (false,true,false);
    phi_current.  evaluate (false,true,false);
    phi_current_s.evaluate (false,true,false);

    for (unsigned int q=0; q<phi_current.n_q_points; ++q)
      {
        // reference configuration:
        const Tensor<2,dim,VectorizedArray<number>>         &grad_u = phi_reference.get_gradient(q);
        const Tensor<2,dim,VectorizedArray<number>>          F      = Physics::Elasticity::Kinematics::F(grad_u);
        const VectorizedArray<number>                        det_F  = determinant(F);
        const Tensor<2,dim,VectorizedArray<number>>          F_bar  = Physics::Elasticity::Kinematics::F_iso(F);
        const SymmetricTensor<2,dim,VectorizedArray<number>> b_bar  = Physics::Elasticity::Kinematics::b(F_bar);

        // current configuration
        const Tensor<2,dim,VectorizedArray<number>>          &grad_Nx_v      = phi_current.get_gradient(q);
        const SymmetricTensor<2,dim,VectorizedArray<number>> &symm_grad_Nx_v = phi_current.get_symmetric_gradient(q);

        SymmetricTensor<2,dim,VectorizedArray<number>> tau;
        material->get_tau(tau,det_F,b_bar);
        const Tensor<2,dim,VectorizedArray<number>> tau_ns (tau);

        const SymmetricTensor<2,dim,VectorizedArray<number>> jc_part = material->act_Jc(det_F,b_bar,symm_grad_Nx_v);

        const VectorizedArray<number> & JxW_current = phi_current.JxW(q);
        VectorizedArray<number> JxW_scale = phi_reference.JxW(q);
        for (unsigned int i = 0; i < VectorizedArray<number>::n_array_elements; ++i)
          if (std::abs(JxW_current[i])>1e-10)
            JxW_scale[i] *= 1./JxW_current[i];

        // This is the $\mathsf{\mathbf{k}}_{\mathbf{u} \mathbf{u}}$
        // contribution. It comprises a material contribution, and a
        // geometrical stress contribution which is only added along
        // the local matrix diagonals:
        phi_current_s.submit_symmetric_gradient(
          jc_part * JxW_scale
          // Note: We need to integrate over the reference element, so the weights have to be adjusted
          ,q);

        // geometrical stress contribution
        const Tensor<2,dim,VectorizedArray<number>> geo = egeo_grad(grad_Nx_v,tau_ns);
        phi_current.submit_gradient(
          geo * JxW_scale
          // Note: We need to integrate over the reference element, so the weights have to be adjusted
          // phi_reference.JxW(q) / phi_current.JxW(q)
          ,q);

      } // end of the loop over quadrature points

    // actually do the contraction
    phi_current.integrate (false,true);
    phi_current_s.integrate (false,true);
  }



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  void
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::
  compute_diagonal()
  {
    typedef Vector<number> VectorType;

    inverse_diagonal_entries.reset(new DiagonalMatrix<VectorType>());
    diagonal_entries.reset(new DiagonalMatrix<VectorType>());
    VectorType &inverse_diagonal_vector = inverse_diagonal_entries->get_vector();
    VectorType &diagonal_vector         = diagonal_entries->get_vector();

    data_current->initialize_dof_vector(inverse_diagonal_vector);
    data_current->initialize_dof_vector(diagonal_vector);

    unsigned int dummy = 0;
    local_diagonal_cell(*data_current, diagonal_vector, dummy,
                     std::make_pair<unsigned int,unsigned int>(0,data_current->n_macro_cells()));

    // data_current->cell_loop (&NeoHookOperator::local_diagonal_cell,
    //                          this, diagonal_vector, dummy);

    // set_constrained_entries_to_one
    {
      const std::vector<unsigned int> &
      constrained_dofs = data_current->get_constrained_dofs();
      for (unsigned int i=0; i<constrained_dofs.size(); ++i)
        diagonal_vector(constrained_dofs[i]) = 1.;
    }

    // calculate inverse:
    inverse_diagonal_vector = diagonal_vector;

    for (unsigned int i=0; i</*inverse_diagonal_vector.local_size()*/inverse_diagonal_vector.size(); ++i)
      if (std::abs(inverse_diagonal_vector/*.local_element*/(i)) > std::sqrt(std::numeric_limits<number>::epsilon()))
        inverse_diagonal_vector/*.local_element*/(i) = 1./inverse_diagonal_vector/*.local_element*/(i);
      else
        inverse_diagonal_vector/*.local_element*/(i) = 1.;

    // inverse_diagonal_vector.update_ghost_values();
    // diagonal_vector.update_ghost_values();

    diagonal_is_available = true;
  }



  template <int dim, int fe_degree, int n_q_points_1d, typename number>
  number
  NeoHookOperator<dim,fe_degree,n_q_points_1d,number>::el (const unsigned int row,
                                             const unsigned int col) const
  {
    Assert (row == col, ExcNotImplemented());
    (void)col;
    Assert (diagonal_is_available == true, ExcNotInitialized());
    return diagonal_entries->get_vector()(row);
  }

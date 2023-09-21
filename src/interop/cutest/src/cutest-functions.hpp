#pragma once

#include "cutest-types.hpp"

#include <alpaqa/cutest/cutest-errors.hpp>
#include <dlfcn.h>
#include <cassert>

/*
CUTEST_cfn      : function and constraints values
CUTEST_cofg     : function value and possibly gradient
CUTEST_cofsg    : function value and possibly gradient in sparse format
CUTEST_ccfg     : constraint functions values and possibly gradients
CUTEST_clfg     : Lagrangian function value and possibly gradient
CUTEST_cgr      : constraints gradients and gradient of objective/Lagrangian function
CUTEST_csgr     : constraints gradients and gradient of objective/Lagrangian function
CUTEST_ccfsg    : constraint functions values and possibly their gradients in sparse format
CUTEST_ccifg    : single constraint function value and possibly its gradient
CUTEST_ccifsg   : single constraint function value and possibly gradient in sparse format
CUTEST_cgrdh    : constraints gradients, Hessian of Lagrangian function and gradient of objective/Lagrangian function
CUTEST_cdh      : Hessian of the Lagrangian
CUTEST_cdhc     : Hessian of the constraint part of the Lagrangian
CUTEST_cshp     : sparsity pattern of the Hessian of the Lagrangian function
CUTEST_csh      : Hessian of the Lagrangian, in sparse format
CUTEST_cshc     : Hessian of the constraint part of the Lagrangian, in sparse format
CUTEST_ceh      : sparse Lagrangian Hessian matrix in finite element format
CUTEST_cifn     : problem function value
CUTEST_cigr     : gradient of a problem function
CUTEST_cisgr    : gradient of a problem function in sparse format
CUTEST_cidh     : Hessian of a problem function
CUTEST_cish     : Hessian of an individual problem function, in sparse format
CUTEST_csgrsh   : constraints gradients, sparse Lagrangian Hessian and the gradient of either the objective/Lagrangian in sparse format
CUTEST_csgreh   : constraint gradients, the Lagrangian Hessian in finite element format and the gradient of either the objective/Lagrangian in sparse format
CUTEST_chprod   : matrix-vector product of a vector with the Hessian matrix of the Lagrangian
CUTEST_cshprod  : matrix-vector product of a sparse vector with the Hessian matrix of the Lagrangian
CUTEST_chcprod  : matrix-vector product of a vector with the Hessian matrix of the constraint part of the Lagrangian
CUTEST_cshcprod : matrix-vector product of a spaarse vector with the Hessian matrix of the constraint part of the Lagrangian
CUTEST_cjprod   : matrix-vector product of a vector with the Jacobian of the constraints, or its transpose
CUTEST_csjprod  : matrix-vector product of a sparse vector with the Jacobian of the constraints, or its transpose
CUTEST_cchprods : matrix-vector products of a vector with each of the Hessian matrices of the constraint functions
*/

namespace alpaqa::cutest {

// clang-format off
using csetup        = Function<"cutest_cint_csetup_", void(integer *status, const integer *funit, const integer *iout, const integer *io_buffer, integer *n, integer *m, doublereal *x, doublereal *bl, doublereal *bu, doublereal *v, doublereal *cl, doublereal *cu, logical *equatn, logical *linear, const integer *e_order, const integer *l_order, const integer *v_order)>;
using cterminate    = Function<"cutest_cterminate_", void(integer *status)>;

using cdimen        = Function<"cutest_cdimen_", void(integer *status, const integer *funit, integer *n, integer *m)>;
using cdimsj        = Function<"cutest_cdimsj_", void(integer *status, integer *nnzj)>;
using cdimsh        = Function<"cutest_cdimsh_", void(integer *status, integer *nnzh)>;

using probname      = Function<"cutest_probname_", void(integer *status, char *pname)>;
using creport       = Function<"cutest_creport_", void(integer *status, doublereal *calls, doublereal *time)>;
using creport       = Function<"cutest_creport_", void(integer *status, doublereal *calls, doublereal *time)>;
using ureport       = Function<"cutest_ureport_", void(integer *status, doublereal *calls, doublereal *time)>;

using cfn           = Function<"cutest_cfn_", void(integer *status, const integer *n, const integer *m, const doublereal *x, doublereal *f, doublereal *c)>;
using cofg          = Function<"cutest_cint_cofg_", void(integer *status, const integer *n, const doublereal *x, doublereal *f, doublereal *g, const logical *grad)>;
using ccfg          = Function<"cutest_cint_ccfg_", void(integer *status, const integer *n, const integer *m, const doublereal *x, doublereal *c, const logical *jtrans, const integer *lcjac1, const integer *lcjac2, doublereal *cjac, const logical *grad)>;
using clfg          = Function<"cutest_cint_clfg_", void(integer *status, const integer *n, const integer *m, const doublereal *x, const doublereal *y, doublereal *f, doublereal *g, const logical *grad)>;
using ccfsg         = Function<"cutest_cint_ccfsg_", void(integer *status, const integer *n, const integer *m, const doublereal *x, doublereal *c, integer *nnzj, const integer *lcjac, doublereal *cjac, integer *indvar, integer *indfun, const logical *grad)>;
using ccifg         = Function<"cutest_cint_ccifg_", void(integer *status, const integer *n, const integer *icon, const doublereal *x, doublereal *ci, doublereal *gci, const logical *grad)>;
using cdh           = Function<"cutest_cdh_", void(integer *status, const integer *n, const integer *m, const doublereal *x, const doublereal *y, const integer *lh1, doublereal *h)>;
using cshp          = Function<"cutest_cshp_", void(integer *status, const integer *n, integer *nnzh, const integer *lh, integer *irnh, integer *icnh)>;
using csh           = Function<"cutest_csh_", void(integer *status, const integer *n, const integer *m, const doublereal *x, const doublereal *y, integer *nnzh, const integer *lh, doublereal *h, integer *irnh, integer *icnh)>;
using cigr          = Function<"cutest_cigr_", void(integer *status, const integer *n, const integer *iprob, const doublereal *x, doublereal *g)>;
using chprod        = Function<"cutest_chprod_", void(integer *status, const integer *n, const integer *m, const logical *goth, const doublereal *x, const doublereal *y, doublereal *p, doublereal *q)>;
using cjprod        = Function<"cutest_cjprod_", void(integer *status, const integer *n, const integer *m, const logical *gotj, const logical *jtrans,const doublereal *x, const doublereal *p, const integer *lp, doublereal *r, const integer *lr)>;
using csjp          = Function<"cutest_csjp_", void(integer *status, integer *nnzj, const integer *lj, integer *J_var, integer *J_con)>;

using fortran_open  = Function<"fortran_open_", void(const integer *funit, const char *fname, integer *ierr)>;
using fortran_close = Function<"fortran_close_", void(const integer *funit, integer *ierr)>;
// clang-format on

template <Name Nm, class Sgn>
auto Function<Nm, Sgn>::load(void *handle) -> signature_t * {
    (void)::dlerror();
    static_assert(name.value.back() == '\0');
    const char *name_cstr = name.value.data();
    auto func = reinterpret_cast<signature_t *>(::dlsym(handle, name_cstr));
    if (const char *error = ::dlerror())
        throw function_load_error(error);
    assert(func);
    return func;
}

} // namespace alpaqa::cutest
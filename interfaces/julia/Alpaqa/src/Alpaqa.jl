module Alpaqa

import LinearAlgebra as LA
import Alpaqa_jll

export InnerSolverResult
export InnerProblem
export EvalCounter
export SolverReturn
export DriverReturn
export DriverResult
export InnerSolverFunction
export Problem
export value_and_gradient
export value
export gradient
export prox!
export hvp!
export jprox!
export hess!
export load_casadi_problem
export load_cutest_problem
export ocp_nx_casadi
export ocp_nu_casadi
export param_size_casadi
export get_param_casadi
export set_param_casadi
export ocp_simulate_casadi
export describe
export print_counters
export reset_counters
export num_variables
export num_constraints
export lagrangian_gradient
export initial_guess
export bounds
export run_alm
export run_alpaqa
export alpaqa_version
export alpaqa_build_time
export alpaqa_commit_hash

mutable struct ClosureData
    callable::Function
end

struct InnerSolverResult
    success::Bool
    num_iter::Cint
    achieved_tolerance::Cdouble
end

struct InnerProblem
    data::Ptr{Cvoid}
    value_and_gradient::Ptr{Cvoid}
    prox::Ptr{Cvoid}
    hvp::Ptr{Cvoid}
    jprox::Ptr{Cvoid}
    hess::Ptr{Cvoid}
    value::Ptr{Cvoid}
    gradient::Ptr{Cvoid}
end

struct EvalCounter
    projecting_difference_constraints::Cuint
    projection_multipliers::Cuint
    proximal_gradient_step::Cuint
    inactive_indices_res_lna::Cuint
    prox_jacobian_diag::Cuint
    nonsmooth_objective::Cuint
    objective::Cuint
    objective_gradient::Cuint
    objective_and_gradient::Cuint
    objective_and_constraints::Cuint
    objective_gradient_and_constraints_gradient_product::Cuint
    constraints::Cuint
    constraints_gradient_product::Cuint
    grad_gi::Cuint
    constraints_jacobian::Cuint
    lagrangian_gradient::Cuint
    lagrangian_hessian_product::Cuint
    lagrangian_hessian::Cuint
    augmented_lagrangian_hessian_product::Cuint
    augmented_lagrangian_hessian::Cuint
    augmented_lagrangian::Cuint
    augmented_lagrangian_gradient::Cuint
    augmented_lagrangian_and_gradient::Cuint
end

struct SolverReturn
    success::Bool
    evaluations::EvalCounter
    num_inner_iter::Cuint
    num_outer_iter::Cuint
    num_outer_fail::Cuint
end

struct DriverReturn
    success::Bool
    evaluations::EvalCounter
    num_inner_iter::Cuint
    num_outer_iter::Cuint
    output_file_id::Ptr{Cchar}
    error::Ptr{Cchar}
end

struct DriverResult
    success::Bool
    evaluations::EvalCounter
    num_inner_iter::Cuint
    num_outer_iter::Cuint
    output_file_id::Union{String, Nothing}
end

function julia_call_inner_solver_thunk(user_data::Ptr{Cvoid}, problem::Ptr{InnerProblem}, x::Ptr{Cdouble}, nx::Csize_t, tol::Cdouble)
    closure_ref = unsafe_pointer_to_objref(user_data)::ClosureData
    problem_ref = unsafe_load(problem)
    x_vec = unsafe_wrap(Array, x, (nx,))
    return closure_ref.callable(problem_ref, x_vec, tol)
end

mutable struct InnerSolverFunction
    context::Ptr{Cvoid}
    func::Ptr{Cvoid}
    closure_data::ClosureData

    function InnerSolverFunction(callable)
        closure_data = ClosureData(callable)
        context_ptr = pointer_from_objref(closure_data)
        thunk = @cfunction(julia_call_inner_solver_thunk, InnerSolverResult,
            (Ptr{Cvoid}, Ptr{InnerProblem}, Ptr{Cdouble}, Csize_t, Cdouble))
        return new(context_ptr, thunk, closure_data)
    end
end

struct CInnerSolverFunction
    context::Ptr{Cvoid}
    func::Ptr{Cvoid}
end

CInnerSolverFunction(inner_solver::InnerSolverFunction) =
    CInnerSolverFunction(inner_solver.context, inner_solver.func)

function with_inner_solver(f, inner_solver::Union{InnerSolverFunction, Nothing})
    if inner_solver === nothing
        return f(C_NULL)
    end
    c_inner_solver = Ref(CInnerSolverFunction(inner_solver))
    GC.@preserve inner_solver c_inner_solver begin
        ptr = Base.unsafe_convert(Ptr{CInnerSolverFunction}, c_inner_solver)
        return f(ptr)
    end
end

function with_options_cstrings(f, options::AbstractVector{<:AbstractString})
    options_str = String[s for s in options]
    options_c = Cstring[pointer(s) for s in options_str]
    GC.@preserve options_str options_c begin
        return f(options_c)
    end
end

function owned_cstring_to_string(c_str::Ptr{Cchar}, name::AbstractString)
    c_str == C_NULL && throw(ErrorException("$name returned NULL"))
    try
        return unsafe_string(c_str)
    finally
        Libc.free(c_str)
    end
end

function value_and_gradient(problem::InnerProblem, x::DenseVector{Cdouble})
    grad = Vector{Cdouble}(undef, length(x))
    fx = @ccall $(problem.value_and_gradient)(problem.data::Ptr{Cvoid}, x::Ptr{Cdouble},
        length(x)::Csize_t, grad::Ptr{Cdouble})::Cdouble
    return fx, grad
end

value_and_gradient(problem::InnerProblem, x::AbstractVector{<:Real}) =
    value_and_gradient(problem, Vector{Cdouble}(x))

function value(problem::InnerProblem, x::DenseVector{Cdouble})
    return @ccall $(problem.value)(problem.data::Ptr{Cvoid}, x::Ptr{Cdouble}, length(x)::Csize_t)::Cdouble
end

value(problem::InnerProblem, x::AbstractVector{<:Real}) =
    value(problem, Vector{Cdouble}(x))

function gradient(problem::InnerProblem, x::DenseVector{Cdouble})
    grad = Vector{Cdouble}(undef, length(x))
    @ccall $(problem.gradient)(problem.data::Ptr{Cvoid}, x::Ptr{Cdouble}, length(x)::Csize_t,
        grad::Ptr{Cdouble})::Cvoid
    return grad
end

gradient(problem::InnerProblem, x::AbstractVector{<:Real}) =
    gradient(problem, Vector{Cdouble}(x))

function prox!(y::DenseVector{Cdouble}, problem::InnerProblem,
    x::DenseVector{Cdouble}, gamma::Real)
    size(y) == size(x) || throw(DimensionMismatch("Dimension mismatch y and x"))
    return @ccall $(problem.prox)(problem.data::Ptr{Cvoid}, gamma::Cdouble, x::Ptr{Cdouble},
        length(x)::Csize_t, y::Ptr{Cdouble})::Cdouble
end

function hvp!(problem::InnerProblem, Hv::DenseVector{Cdouble},
    x::DenseVector{Cdouble}, v::DenseVector{Cdouble})
    size(v) == size(x) || throw(DimensionMismatch("Dimension mismatch v and x"))
    size(Hv) == size(x) || throw(DimensionMismatch("Dimension mismatch Hv and x"))
    @ccall $(problem.hvp)(problem.data::Ptr{Cvoid}, x::Ptr{Cdouble}, length(x)::Csize_t,
        v::Ptr{Cdouble}, Hv::Ptr{Cdouble})::Cvoid
end

function jprox!(problem::InnerProblem, Jac::DenseMatrix{Cdouble},
    x::DenseVector{Cdouble}, gamma::Real)
    Jdiag = Vector{Cdouble}(undef, length(x))
    @ccall $(problem.jprox)(problem.data::Ptr{Cvoid}, gamma::Cdouble, x::Ptr{Cdouble},
        length(x)::Csize_t, Jdiag::Ptr{Cdouble})::Cvoid
    Jac .= LA.diagm(Jdiag)
end

function hess!(problem::InnerProblem, H::DenseMatrix{Cdouble},
    x::DenseVector{Cdouble})
    @ccall $(problem.hess)(problem.data::Ptr{Cvoid}, x::Ptr{Cdouble}, length(x)::Csize_t,
        H::Ptr{Cdouble})::Cvoid
end

mutable struct Problem
    problem::Ptr{Cvoid}

    function Problem(problem)
        p = new(problem)
        finalizer(p) do obj
            @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_free_problem(obj.problem::Ptr{Cvoid})::Cvoid
        end
        return p
    end
end

function load_casadi_problem(file::AbstractString,
    options::AbstractVector{<:AbstractString}=String[])
    return with_options_cstrings(options) do options_c
        return Problem(@ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_load_problem("cs"::Cstring,
            file::Cstring, length(options_c)::Csize_t,
            options_c::Ptr{Cstring})::Ptr{Cvoid})
    end
end

function load_cutest_problem(file::AbstractString,
    options::AbstractVector{<:AbstractString}=String[])
    return with_options_cstrings(options) do options_c
        return Problem(@ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_load_problem("cu"::Cstring,
            file::Cstring, length(options_c)::Csize_t,
            options_c::Ptr{Cstring})::Ptr{Cvoid})
    end
end

function ocp_nx_casadi(problem::Problem)
    @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_ocp_nx_casadi(problem.problem::Ptr{Cvoid})::Csize_t
end

function ocp_nu_casadi(problem::Problem)
    @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_ocp_nu_casadi(problem.problem::Ptr{Cvoid})::Csize_t
end

function param_size_casadi(problem::Problem)
    @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_param_size_casadi(problem.problem::Ptr{Cvoid})::Csize_t
end

function get_param_casadi(problem::Problem, param::DenseVector{Cdouble})
    @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_get_param_casadi(problem.problem::Ptr{Cvoid}, param::Ptr{Cdouble},
        length(param)::Csize_t)::Cvoid
end

function get_param_casadi(problem::Problem)
    n = param_size_casadi(problem)
    param = Vector{Cdouble}(undef, n)
    get_param_casadi(problem, param)
    return param
end

function set_param_casadi(problem::Problem, param::DenseVector{Cdouble})
    @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_set_param_casadi(problem.problem::Ptr{Cvoid}, param::Ptr{Cdouble},
        length(param)::Csize_t)::Cvoid
end

set_param_casadi(problem::Problem, param::AbstractVector{<:Real}) =
    set_param_casadi(problem, Vector{Cdouble}(param))

function ocp_simulate_casadi(problem::Problem, x0::DenseVector{Cdouble},
    u::DenseVector{Cdouble})
    x1 = Vector{Cdouble}(undef, length(x0))
    @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_ocp_simulate_casadi(problem.problem::Ptr{Cvoid},
        x0::Ptr{Cdouble}, length(x0)::Csize_t, u::Ptr{Cdouble}, length(u)::Csize_t,
        x1::Ptr{Cdouble}, length(x1)::Csize_t)::Cvoid
    return x1
end

ocp_simulate_casadi(problem::Problem, x0::AbstractVector{<:Real}, u::AbstractVector{<:Real}) =
    ocp_simulate_casadi(problem, Vector{Cdouble}(x0), Vector{Cdouble}(u))

function describe(problem::Problem)
    @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_describe_problem(problem.problem::Ptr{Cvoid})::Cvoid
end

function print_counters(problem::Problem)
    @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_print_counters_problem(problem.problem::Ptr{Cvoid})::Cvoid
end

function reset_counters(problem::Problem)
    @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_reset_counters_problem(problem.problem::Ptr{Cvoid})::Cvoid
end

function num_variables(problem::Problem)
    @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_num_variables(problem.problem::Ptr{Cvoid})::Csize_t
end

function num_constraints(problem::Problem)
    @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_num_constraints(problem.problem::Ptr{Cvoid})::Csize_t
end

function lagrangian_gradient(problem::Problem, x::DenseVector{Cdouble},
    y::DenseVector{Cdouble})
    grad = Vector{Cdouble}(undef, length(x))
    @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_lagrangian_gradient(problem.problem::Ptr{Cvoid},
        x::Ptr{Cdouble}, length(x)::Csize_t, y::Ptr{Cdouble}, length(y)::Csize_t,
        grad::Ptr{Cdouble})::Cvoid
    return grad
end

lagrangian_gradient(problem::Problem, x::AbstractVector{<:Real}, y::AbstractVector{<:Real}) =
    lagrangian_gradient(problem, Vector{Cdouble}(x), Vector{Cdouble}(y))

function initial_guess(problem::Problem)
    n = num_variables(problem)
    m = num_constraints(problem)
    x0 = Vector{Cdouble}(undef, n)
    y0 = Vector{Cdouble}(undef, m)
    @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_initial_guess(problem.problem::Ptr{Cvoid},
        x0::Ptr{Cdouble}, n::Csize_t, y0::Ptr{Cdouble}, m::Csize_t)::Cvoid
    return x0, y0
end

function bounds(problem::Problem)
    n = num_variables(problem)
    xl = Vector{Cdouble}(undef, n)
    xu = Vector{Cdouble}(undef, n)
    @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_bounds(problem.problem::Ptr{Cvoid},
        xl::Ptr{Cdouble}, xu::Ptr{Cdouble}, n::Csize_t)::Cvoid
    return xl, xu
end

function run_alm(problem::Problem, inner_solver::InnerSolverFunction,
    x::DenseVector{Cdouble}, y::DenseVector{Cdouble},
    options::AbstractVector{<:AbstractString}=String[])
    return with_inner_solver(inner_solver) do c_inner_solver
        return with_options_cstrings(options) do options_c
            return @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_run_alm(problem.problem::Ptr{Cvoid},
                c_inner_solver::Ptr{CInnerSolverFunction},
                x::Ptr{Cdouble}, length(x)::Csize_t,
                y::Ptr{Cdouble}, length(y)::Csize_t,
                length(options_c)::Csize_t, options_c::Ptr{Cstring})::SolverReturn
        end
    end
end

function run_alpaqa(problem_path::String, problem_type::String, problem::Union{Problem, Nothing},
    inner_solver::Union{InnerSolverFunction, Nothing}, x::DenseVector{Cdouble},
    y::DenseVector{Cdouble}, options::AbstractVector{<:AbstractString}=String[])
    ret = with_inner_solver(inner_solver) do c_inner_solver
        return with_options_cstrings(options) do options_c
            return @ccall Alpaqa_jll.alpaqa_jl.alpaqa_jl_run_alpaqa(
                problem_path::Cstring,
                problem_type::Cstring,
                (problem === nothing ? C_NULL : problem.problem)::Ptr{Cvoid},
                c_inner_solver::Ptr{CInnerSolverFunction},
                x::Ptr{Cdouble}, length(x)::Csize_t,
                y::Ptr{Cdouble}, length(y)::Csize_t,
                length(options_c)::Csize_t, options_c::Ptr{Cstring}
            )::DriverReturn
        end
    end
    if ret.error != C_NULL
        msg = try
            unsafe_string(ret.error)
        finally
            Libc.free(ret.error)
        end
        throw(ErrorException(msg))
    end
    fid = nothing
    if ret.output_file_id != C_NULL
        fid = try
            unsafe_string(ret.output_file_id)
        finally
            Libc.free(ret.output_file_id)
        end
    end
    return DriverResult(ret.success, ret.evaluations, ret.num_inner_iter, ret.num_outer_iter, fid)
end

function run_alpaqa(problem_path::String, problem_type::String,
    inner_solver::Union{InnerSolverFunction, Nothing}, x::DenseVector{Cdouble},
    y::DenseVector{Cdouble}, options::AbstractVector{<:AbstractString}=String[])
    return run_alpaqa(problem_path, problem_type, nothing, inner_solver, x, y, options)
end

function run_alpaqa(problem::Problem, inner_solver::Union{InnerSolverFunction, Nothing},
    x::DenseVector{Cdouble}, y::DenseVector{Cdouble},
    options::AbstractVector{<:AbstractString}=String[])
    return run_alpaqa("", "", problem, inner_solver, x, y, options)
end

function alpaqa_version()
    return owned_cstring_to_string(
        @ccall(Alpaqa_jll.alpaqa_jl.alpaqa_jl_version()::Ptr{Cchar}),
        "alpaqa_jl_version",
    )
end

function alpaqa_build_time()
    return owned_cstring_to_string(
        @ccall(Alpaqa_jll.alpaqa_jl.alpaqa_jl_build_time()::Ptr{Cchar}),
        "alpaqa_jl_build_time",
    )
end

function alpaqa_commit_hash()
    return owned_cstring_to_string(
        @ccall(Alpaqa_jll.alpaqa_jl.alpaqa_jl_commit_hash()::Ptr{Cchar}),
        "alpaqa_jl_commit_hash",
    )
end

end

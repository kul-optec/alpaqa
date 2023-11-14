function varargout = minimize(problem, x0, y0, options)
%minimize - Solve the given minimization problem.
arguments
    problem (1,1) alpaqa.Problem
    x0 (1,:) double = []
    y0 (1,:) double = []
    options.method (1,1) string = "panoc"
    options.params (1,1) {mustBeStructOrString} = struct
end

if isa(options.params, 'struct')
    params_str = jsonencode(options.params, 'ConvertInfAndNaN', true);
else
    params_str = convertStringsToChars(options.params);
end

f = casadi.Function('f', {problem.x', problem.param'}, {problem.f});
g = casadi.Function('g', {problem.x', problem.param'}, {problem.g'});

problem_struct = struct;
problem_struct.f = f.serialize();
problem_struct.g = g.serialize();
problem_struct.C_lb = problem.C_lowerbound;
problem_struct.C_ub = problem.C_upperbound;
problem_struct.D_lb = problem.D_lowerbound;
problem_struct.D_ub = problem.D_upperbound;
problem_struct.l1_reg = problem.l1_regularization;
problem_struct.param = problem.param_value;

[varargout{1:nargout}] = alpaqa.alpaqa_mex('minimize', ...
    problem_struct, x0, y0, convertStringsToChars(options.method), params_str);

end

function mustBeStructOrString(v)
mustBeA(v, ["string", "char", "struct"]);
end

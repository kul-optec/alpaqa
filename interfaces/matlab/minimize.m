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

[varargout{1:nargout}] = alpaqa.alpaqa_mex('minimize', ...
    problem, x0, y0, convertStringsToChars(options.method), params_str);

end

function mustBeStructOrString(v)
mustBeA(v, ["string", "char", "struct"]);
end

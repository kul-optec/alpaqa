%% Build the problem (CasADi code, independent of alpaqa)
import casadi.*

% Make symbolic decision variables
x1 = SX.sym('x1'); x2 = SX.sym('x2');
x = [x1, x2];  % Collect decision variables into one vector
% Make a parameter symbol
p = SX.sym('p');

% Objective function f and the constraints function g
problem = alpaqa.Problem;
problem.x = x;
problem.param = p;
problem.f = (1 - x1)^2 + p * (x2 - x1^2)^2;
problem.g = [(x1 - 0.5)^3 - x2 + 1, x1 + x2 - 1.5];

% Define the bounds
problem.C_lowerbound = [-0.25, -0.5];  % -0.25 <= x1 <= 1.5, -0.5 <= x2 <= 2.5
problem.C_upperbound = [1.5, 2.5];
problem.D_lowerbound = [-inf, -inf];  %     g1 <= 0,           g2 <= 0
problem.D_upperbound = [0, 0];
problem.param_value = 10.0;

%% Choose a solver and configure its parameters
solver = "panoc.lbfgs";

params = struct;
params.accel.memory = 10;
params.solver.stop_crit = "FPRNorm";
params.solver.print_interval = 1;
params.alm.tolerance = 1e-10;
params.alm.dual_tolerance = 1e-10;
params.alm.initial_penalty = 50;
params.alm.penalty_update_factor = 20;
params.alm.print_interval = 1;
params.alm.max_iter = 20;

%% Compute a solution

[x_sol, y_sol, stats] = alpaqa.minimize( ...
    problem, method=solver, params=params);

%% Compute a solution starting with an initial guess

% Set initial guesses at arbitrary values
x0 = [0.1, 1.8];  % decision variables
y0 = [0.0, 0.0];  % Lagrange multipliers for g(x)

% Solve the problem
[x_sol, y_sol, stats] = alpaqa.minimize( ...
    problem, x0, y0, method=solver, params=params);

%% Print the results
disp(stats.status)
disp('Solution:');
disp(x_sol);
disp('Multipliers:');
disp(y_sol);

classdef Problem
    properties
        f (1,1) {mustBeCasADiExpression} = casadi.SX.sym('f');
        x (1,:) {mustBeCasADiExpression} = casadi.SX.sym('x');
        g (1,:) {mustBeCasADiExpression} = casadi.SX.sym('g');
        param (1,:) {mustBeCasADiExpression} = casadi.SX.sym('param');
        C_lowerbound (1,:) double = [];
        C_upperbound (1,:) double = [];
        D_lowerbound (1,:) double = [];
        D_upperbound (1,:) double = [];
        l1_regularization (1,:) double = [];
        param_value (1,:) double = [];
    end
end

function mustBeCasADiExpression(v)
mustBeA(v, ["casadi.SX", "casadi.MX"]);
end

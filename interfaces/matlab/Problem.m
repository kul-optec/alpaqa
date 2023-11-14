classdef Problem
    properties
        f (1,1) {mustBeCasADiExpression} = casadi.SX.sym('f');
        x (1,1) {mustBeCasADiExpression} = casadi.SX.sym('x');
        g (1,1) {mustBeCasADiExpression} = casadi.SX.sym('g');
        p (1,1) {mustBeCasADiExpression} = casadi.SX.sym('p');
        C_lowerbound (1,:) double = [];
        C_upperbound (1,:) double = [];
        D_lowerbound (1,:) double = [];
        D_upperbound (1,:) double = [];
        l1_regularization (1,:) double = [];
        param (1,:) double = [];
    end
end

function mustBeCasADiExpression(v)
mustBeA(v, ["casadi.SX", "casadi.MX"]);
end

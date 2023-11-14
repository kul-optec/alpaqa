function [v, varargout] = version()
%version - Return the alpaqa version, build time, and commit hash.

[v, varargout{1:max(nargout, 1) - 1}] = alpaqa.alpaqa_mex('version');
end
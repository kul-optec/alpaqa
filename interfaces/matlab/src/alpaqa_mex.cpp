#include <alpaqa/util/demangled-typename.hpp>
#include <alpaqa-mex-types.hpp>

namespace alpaqa::mex {
SolverResults minimize(crvec x0, crvec y0, std::string_view method,
                       const Options &options);
} // namespace alpaqa::mex

#include <mex.hpp>
#include <mexAdapter.hpp>

#include <exception>
#include <map>
#include <memory>
#include <string>
#include <string_view>

using namespace matlab::mex;
using namespace matlab::data;

class MexFunction : public Function {
  private:
    std::shared_ptr<matlab::engine::MATLABEngine> matlabPtr;

  public:
    MexFunction() : matlabPtr{getEngine()} {}

    // Generate an error displaying the given message.
    void displayError(const std::string &errorMessage) {
        ArrayFactory factory;
        matlabPtr->feval(
            u"error", 0,
            std::vector<Array>({factory.createScalar(errorMessage)}));
    }

    // Main entry-point for the MEX-file.
    void operator()(ArgumentList outputs, ArgumentList inputs) override {
        std::u16string command;
        // Check arguments
        if (inputs.size() < 1) {
            displayError("At least one input required.");
        }
        if (outputs.size() > 3) {
            displayError("Too many outputs specified.");
        }
        if (inputs[0].getType() == ArrayType::CHAR) {
            command = static_cast<CharArray>(inputs[0]).toUTF16();
        } else {
            displayError("First argument must be a character array.");
        }

        // First argument is a string that determines action to take
        using handler_t = void (MexFunction::*)(ArgumentList, ArgumentList);
        std::map<std::u16string_view, handler_t> handlers{
            {u"minimize", &MexFunction::call_minimize},
        };
        if (auto handler = handlers.find(command); handler != handlers.end())
            (this->*handler->second)(outputs, inputs);
        else
            displayError("Invalid command");
    }

    // Solve the given minimization problem.
    void call_minimize(ArgumentList outputs, ArgumentList inputs) try {
        // x0
        if (inputs[2].getType() != ArrayType::DOUBLE) {
            displayError("Third argument (x0) must be an array of double.");
        }
        matlab::data::TypedArray<double> x0 = std::move(inputs[2]);
        alpaqa::mex::vec x0vec(x0.getNumberOfElements());
        std::copy(x0.begin(), x0.end(), x0vec.begin());
        // y0
        if (inputs[3].getType() != ArrayType::DOUBLE) {
            displayError("Fourth argument (y0) must be an array of double.");
        }
        matlab::data::TypedArray<double> y0 = std::move(inputs[3]);
        alpaqa::mex::vec y0vec(y0.getNumberOfElements());
        std::copy(y0.begin(), y0.end(), y0vec.begin());
        // method
        if (inputs[4].getType() != ArrayType::CHAR) {
            displayError("Fifth argument (method) must be a character array.");
        }
        auto method = static_cast<CharArray>(inputs[4]).toAscii();
        // params
        if (inputs[5].getType() != ArrayType::CHAR) {
            displayError("Sixth argument (params) must be a character array.");
        }
        auto params_str = static_cast<CharArray>(inputs[5]).toAscii();
        auto options    = nlohmann::json::parse(params_str);

        // Run solver
        auto results = alpaqa::mex::minimize(x0vec, y0vec, method, options);

        // Return output
        ArrayFactory factory;
        auto n = static_cast<size_t>(results.x.size());
        auto m = static_cast<size_t>(results.y.size());
        if (outputs.size() >= 1)
            outputs[0] =
                factory.createArray({1, n}, results.x.begin(), results.x.end());
        if (outputs.size() >= 2)
            outputs[1] =
                factory.createArray({1, m}, results.y.begin(), results.y.end());
        if (outputs.size() >= 3)
            outputs[2] =
                matlabPtr->feval(u"jsondecode", 1,
                                 std::vector<Array>{factory.createCharArray(
                                     to_string(results.stats))})[0];
    } catch (std::exception &e) {
        displayError(demangled_typename(typeid(e)) + ": " + e.what());
    }
};
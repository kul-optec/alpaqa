#include <alpaqa/util/demangled-typename.hpp>
#include <MatlabDataArray/ArrayType.hpp>
#include <alpaqa-version.h>
#include <alpaqa-mex-types.hpp>

#include <mex.hpp>
#include <mexAdapter.hpp>

#include <exception>
#include <map>
#include <memory>
#include <string>

using matlab::data::Array;
using matlab::data::ArrayFactory;
using matlab::data::ArrayType;
using matlab::data::CharArray;
using matlab::data::StructArray;
using matlab::data::TypedArray;
using matlab::mex::ArgumentList;

class MexFunction : public matlab::mex::Function {
  private:
    std::shared_ptr<matlab::engine::MATLABEngine> matlabPtr;

  public:
    MexFunction() : matlabPtr{getEngine()} {}

    // Generate an error displaying the given message.
    void displayError(std::u16string errorMessage) {
        ArrayFactory factory;
        matlabPtr->feval(u"error", 0,
                         std::vector<Array>({factory.createCharArray(
                             std::move(errorMessage))}));
    }

    // Get a function that writes to the MATLAB console.
    [[nodiscard]] auto utf8_writer() const {
        return [matlabPtr{matlabPtr}](std::string_view in) {
            ArrayFactory factory;
            matlabPtr->feval(u"fprintf", 0,
                             std::vector<Array>({factory.createCharArray(
                                 alpaqa::mex::utf8to16(in))}));
        };
    }

    // Main entry-point for the MEX-file.
    void operator()(ArgumentList outputs, ArgumentList inputs) override {
        // Check arguments
        if (inputs.size() < 1)
            displayError(u"At least one input required.");
        if (inputs[0].getType() != ArrayType::CHAR)
            displayError(u"First argument must be a character array.");
        auto command = static_cast<CharArray>(inputs[0]).toUTF16();

        // First argument is a string that determines action to take
        using handler_t = void (MexFunction::*)(ArgumentList, ArgumentList);
        std::map<std::u16string_view, handler_t> handlers{
            {u"minimize", &MexFunction::call_minimize},
            {u"version", &MexFunction::call_version},
        };
        if (auto handler = handlers.find(command); handler != handlers.end())
            (this->*handler->second)(outputs, inputs);
        else
            displayError(u"Invalid command");
    }

    // Solve the given minimization problem.
    void call_version(ArgumentList outputs, ArgumentList) noexcept {
        if (outputs.size() > 3)
            displayError(u"version returns at most three outputs.");
        ArrayFactory factory;
        if (outputs.size() >= 1)
            outputs[0] = factory.createCharArray(ALPAQA_VERSION_FULL);
        if (outputs.size() >= 2)
            outputs[1] = factory.createCharArray(alpaqa_build_time);
        if (outputs.size() >= 3)
            outputs[2] = factory.createCharArray(alpaqa_commit_hash);
    }

    // Solve the given minimization problem.
    void call_minimize(ArgumentList outputs, ArgumentList inputs) try {
        using namespace alpaqa::mex;
        using nlohmann::json;
        if (inputs.size() < 6)
            displayError(u"minimize requires six arguments.");
        if (outputs.size() > 3)
            displayError(u"minimize returns at most three outputs.");
        // problem
        if (inputs[1].getType() != ArrayType::STRUCT ||
            inputs[1].getNumberOfElements() != 1) {
            displayError(u"Second argument (problem) must be a scalar struct");
        }
        StructArray problem_structs = inputs[1];
        auto fields                 = problem_structs.getFieldNames();
        auto get_field = [&](const std::string &name, ArrayType type) {
            auto found = std::find(fields.begin(), fields.end(), name);
            if (found == fields.end())
                displayError(u"Missing field " + utf8to16(name));
            Array field = problem_structs[0][*found];
            if (field.getType() != type)
                displayError(u"Incorrect type for field " + utf8to16(name));
            return field;
        };
        ProblemDescription problem;
        problem.f = CharArray{get_field("f", ArrayType::CHAR)}.toAscii();
        problem.g = CharArray{get_field("g", ArrayType::CHAR)}.toAscii();
        TypedArray<double> C_lb   = get_field("C_lb", ArrayType::DOUBLE);
        problem.C_lb              = std::vector(C_lb.begin(), C_lb.end());
        TypedArray<double> C_ub   = get_field("C_ub", ArrayType::DOUBLE);
        problem.C_ub              = std::vector(C_ub.begin(), C_ub.end());
        TypedArray<double> D_lb   = get_field("D_lb", ArrayType::DOUBLE);
        problem.D_lb              = std::vector(D_lb.begin(), D_lb.end());
        TypedArray<double> D_ub   = get_field("D_ub", ArrayType::DOUBLE);
        problem.D_ub              = std::vector(D_ub.begin(), D_ub.end());
        TypedArray<double> l1_reg = get_field("l1_reg", ArrayType::DOUBLE);
        problem.l1_reg            = std::vector(l1_reg.begin(), l1_reg.end());
        TypedArray<double> param  = get_field("param", ArrayType::DOUBLE);
        problem.param             = std::vector(param.begin(), param.end());
        // x0
        if (inputs[2].getType() != ArrayType::DOUBLE) {
            displayError(u"Third argument (x0) must be an array of double.");
        }
        matlab::data::TypedArray<double> x0a = std::move(inputs[2]);
        std::vector<double> x0(x0a.begin(), x0a.end());
        // y0
        if (inputs[3].getType() != ArrayType::DOUBLE)
            displayError(u"Fourth argument (y0) must be an array of double.");
        matlab::data::TypedArray<double> y0a = std::move(inputs[3]);
        std::vector<double> y0(y0a.begin(), y0a.end());
        // method
        if (inputs[4].getType() != ArrayType::CHAR)
            displayError(u"Fifth argument (method) must be a character array.");
        auto method = utf16to8(static_cast<CharArray>(inputs[4]).toUTF16());
        // params
        if (inputs[5].getType() != ArrayType::CHAR)
            displayError(u"Sixth argument (params) must be a character array.");
        auto params_str = utf16to8(static_cast<CharArray>(inputs[5]).toUTF16());
        auto opts       = json::parse(params_str);

        // Run solver
        auto results = minimize(problem, x0, y0, method, opts, utf8_writer());

        // Return output
        ArrayFactory factory;
        if (outputs.size() >= 1) {
            auto n = static_cast<size_t>(results.x.size());
            outputs[0] =
                factory.createArray({1, n}, results.x.begin(), results.x.end());
        }
        if (outputs.size() >= 2) {
            auto m = static_cast<size_t>(results.y.size());
            outputs[1] =
                factory.createArray({1, m}, results.y.begin(), results.y.end());
        }
        if (outputs.size() >= 3) {
            auto stats_str =
                factory.createCharArray(utf8to16(to_string(results.stats)));
            outputs[2] = matlabPtr->feval(
                u"jsondecode", 1, std::vector<Array>{std::move(stats_str)})[0];
        }
    } catch (std::exception &e) {
        using namespace alpaqa::mex;
        displayError(utf8to16(demangled_typename(typeid(e)) + ": " + e.what()));
    }
};

#pragma once

#include <Functions/IFunctionAdaptors.h>
#include <Common/IFactoryWithAliases.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>


namespace DB
{

class Context;


/** Creates function by name.
  * Function could use for initialization (take ownership of shared_ptr, for example)
  *  some dictionaries from Context.
  */
class FunctionFactory : private boost::noncopyable, public IFactoryWithAliases<std::function<FunctionOverloadResolverImplPtr(const Context &)>>
{
public:
    static FunctionFactory & instance();

    template <typename Function>
    void registerFunction(CaseSensitiveness case_sensitiveness = CaseSensitive)
    {
        registerFunction<Function>(Function::name, case_sensitiveness);
    }

    template <typename Function>
    void registerFunction(const std::string & name, CaseSensitiveness case_sensitiveness = CaseSensitive)
    {
        if constexpr (std::is_base_of<IFunction, Function>::value)
            registerFunction(name, &createDefaultFunction<Function>, case_sensitiveness);
        else
            registerFunction(name, &Function::create, case_sensitiveness);
    }

    /// This function is used by YQL - internal Yandex product that depends on ClickHouse by source code.
    std::vector<std::string> getAllNames() const;

    /// Throws an exception if not found.
    FunctionOverloadResolverPtr get(const std::string & name, const Context & context) const;

    /// Returns nullptr if not found.
    FunctionOverloadResolverPtr tryGet(const std::string & name, const Context & context) const;

    /// The same methods to get developer interface implementation.
    FunctionOverloadResolverImplPtr getImpl(const std::string & name, const Context & context) const;
    FunctionOverloadResolverImplPtr tryGetImpl(const std::string & name, const Context & context) const;

private:
    using Functions = std::unordered_map<std::string, Creator>;

    Functions functions;
    Functions case_insensitive_functions;

    template <typename Function>
    static FunctionOverloadResolverImplPtr createDefaultFunction(const Context & context)
    {
        return std::make_unique<DefaultOverloadResolver>(Function::create(context));
    }

    const Functions & getCreatorMap() const override { return functions; }

    const Functions & getCaseInsensitiveCreatorMap() const override { return case_insensitive_functions; }

    String getFactoryName() const override { return "FunctionFactory"; }

    /// Register a function by its name.
    /// No locking, you must register all functions before usage of get.
    void registerFunction(
            const std::string & name,
            Creator creator,
            CaseSensitiveness case_sensitiveness = CaseSensitive);
};

}

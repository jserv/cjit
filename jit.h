#pragma once

#include <string>

namespace cjit {
struct CompiledInfo {
    long *id;
    void *binary;
};

class JitCompiler {
public:
    virtual ~JitCompiler() {}

    virtual void importSymbol(const std::string &name, void *ptr) = 0;
    virtual CompiledInfo compile(const std::string &c_code) = 0;
};

class MirCompiler;

// use like: cjit::create<MirCompiler>();
template <class T>
JitCompiler *create(void);

}  // namespace cjit

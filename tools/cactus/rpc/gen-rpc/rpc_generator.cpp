#include "rpc_generator.h"

#include <cstddef>
#include <ranges>
#include <memory>
#include <map>
#include <stdexcept>
#include <ranges>
#include <string>

namespace {

template <class T>
auto ToUniquePtr(T* p) {
    return std::unique_ptr<T>{p};
}

}  // namespace

bool RpcGenerator::Generate(const google::protobuf::FileDescriptor* file, const std::string&,
                            google::protobuf::compiler::GeneratorContext* generator_context,
                            std::string*) const {
    auto name = file->name();
    if (!name.ends_with(".proto")) {
        throw std::runtime_error{"Bad file extention"};
    }
    name.resize(name.size() - 6);

    auto out_h_includes = ToUniquePtr(generator_context->OpenForInsert(name + ".pb.h", "includes"));
    google::protobuf::io::Printer pb_includes{out_h_includes.get(), '$'};
    pb_includes.Print("#include <cactus/rpc/rpc.h>\n");
    pb_includes.Print("#include <memory>\n");

    auto out_h = ToUniquePtr(generator_context->OpenForInsert(name + ".pb.h", "namespace_scope"));
    google::protobuf::io::Printer pb_h{out_h.get(), '$'};

    for (const auto i : std::views::iota(0, file->service_count())) {
        const auto* service = file->service(i);

        std::map<std::string, std::string> vars{{"class_name", service->name() + "Client"}};

        pb_h.Print(vars, "class $class_name$ {\n");
        pb_h.Print("public:\n");
        pb_h.Indent();
        pb_h.Print(vars, "explicit $class_name$(std::unique_ptr<cactus::IRpcChannel> channel)\n");
        pb_h.Print("    : channel_{std::move(channel)} {}\n");

        if (service->method_count() > 0) {
            pb_h.Print("\n");
            for (const size_t j : std::views::iota(0, service->method_count())) {
                const auto* method = service->method(j);
                vars["method_name"] = method->name();
                vars["method_input_type"] = method->input_type()->name();
                vars["method_output_type"] = method->output_type()->name();

                pb_h.Print(vars,
                           "$method_output_type$ $method_name$(const $method_input_type$& req);\n");
            }
        }

        pb_h.Outdent();
        pb_h.Print("\n");
        pb_h.Print("private:\n");
        pb_h.Indent();
        pb_h.Print("std::unique_ptr<cactus::IRpcChannel> channel_;\n");
        pb_h.Outdent();
        pb_h.Print("};\n");
        pb_h.Print("\n");

        vars["class_name"] = service->name() + "Handler";
        pb_h.Print(vars, "class $class_name$ : public cactus::IRpcService {\n");
        pb_h.Print("public:\n");
        pb_h.Indent();
        pb_h.Print("void CallMethod(const google::protobuf::MethodDescriptor* method,\n");
        pb_h.Print((std::string(8, '\t') + "const google::protobuf::Message& request,\n").c_str());
        pb_h.Print(
            (std::string(8, '\t') + "google::protobuf::Message* response) override;\n").c_str());
        pb_h.Print("\n");

        if (service->method_count() > 0) {
            pb_h.Print("\n");
            for (const size_t j : std::views::iota(0, service->method_count())) {
                const auto method = service->method(j);
                vars["method_name"] = method->name();
                vars["method_input_type"] = method->input_type()->name();
                vars["method_output_type"] = method->output_type()->name();

                pb_h.Print(vars,
                           "virtual void $method_name$(const $method_input_type$& req, "
                           "$method_output_type$* rsp) = 0;\n");
            }
        }

        pb_h.Print("\n");
        pb_h.Print("const google::protobuf::ServiceDescriptor* ServiceDescriptor() override;\n");
        pb_h.Outdent();
        pb_h.Print("};\n");
    }

    auto out_cc = ToUniquePtr(generator_context->OpenForInsert(name + ".pb.cc", "namespace_scope"));
    google::protobuf::io::Printer pb_cc{out_cc.get(), '$'};

    for (const auto i : std::views::iota(0, file->service_count())) {
        const auto* service = file->service(i);
        std::map<std::string, std::string> vars{{"service_name", service->name()}};
        vars["class_name"] = service->name() + "Client";
        if (service->method_count() > 0) {
            pb_cc.Print("\n");
            for (const size_t j : std::views::iota(0, service->method_count())) {
                const auto* method = service->method(j);
                vars["method_name"] = method->name();
                vars["method_input_type"] = method->input_type()->name();
                vars["method_output_type"] = method->output_type()->name();

                pb_cc.Print(vars,
                            "$method_output_type$ $class_name$::$method_name$(const "
                            "$method_input_type$& req) {");
                pb_cc.Indent();
                pb_cc.Print(vars, "$method_output_type$ rsp;\n");
                pb_cc.Print(vars,
                            "const auto* method_descr = "
                            "$service_name$::descriptor()->FindMethodByName(\"$method_name$\");\n");
                pb_cc.Print("channel_->CallMethod(method_descr, req, &rsp);\n");
                pb_cc.Print("return rsp;\n");
                pb_cc.Outdent();
                pb_cc.Print("};\n");
                pb_cc.Print("\n");
            }
        }

        vars["class_name"] = service->name() + "Handler";
        pb_cc.Print(
            vars,
            "void $class_name$::CallMethod(const google::protobuf::MethodDescriptor* method,\n");
        pb_cc.Print(
            (std::string(18, '\t') + "const google::protobuf::Message& request,\n").c_str());
        pb_cc.Print((std::string(18, '\t') + "google::protobuf::Message* response) {\n").c_str());
        if (service->method_count() > 0) {
            pb_cc.Indent();
            for (const size_t j : std::views::iota(0, service->method_count())) {
                const auto* method = service->method(j);
                vars["method_name"] = method->name();
                vars["method_input_type"] = method->input_type()->name();
                vars["method_output_type"] = method->output_type()->name();

                if (j == 0) {
                    pb_cc.Print(vars, "if (method->name() == \"$method_name$\") {\n");
                } else {
                    pb_cc.Print(vars, "} else if (method->name() == \"$method_name$\") {\n");
                }

                pb_cc.Indent();
                pb_cc.Print(vars,
                            "$method_name$(static_cast<const $method_input_type$&>(request), "
                            "static_cast<$method_output_type$*>(response));\n");
                pb_cc.Outdent();
            }
            pb_cc.Print("}\n");
            pb_cc.Outdent();
        }

        pb_cc.Print("}\n");
        pb_cc.Print("\n");

        pb_cc.Print(
            vars,
            "const google::protobuf::ServiceDescriptor* $class_name$::ServiceDescriptor() {\n");
        pb_cc.Indent();
        pb_cc.Print(vars, "return $service_name$::descriptor();\n");
        pb_cc.Outdent();
        pb_cc.Print("}\n");
    }

    return true;
}

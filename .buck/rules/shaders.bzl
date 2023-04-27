def shader_data_impl(ctx: "context") -> ["provider"]:
    hpp = ctx.actions.write(
        ctx.attrs.header,
        ["#pragma once\n\nnamespace {} {{\n".format(ctx.attrs.namespace)] + 
        ["extern const char* const {0}_vert;\nextern const char* const {0}_frag;\n\n".format(prog) for prog in ctx.attrs.programs] + 
        ["}\n"],
    )
    gpp = ctx.actions.write(
        "{}.gpp".format(ctx.attrs.name),
        ['#include "{0}"\n\nnamespace {1} {{\n'.format(ctx.attrs.header, ctx.attrs.namespace)] + 
        ['const char* const {0}_vert = R"(\n@include({0}.vert)\n)";\nconst char* const {0}_frag = R"(\n@include({0}.frag)\n)";\n\n'.format(prog) for prog in ctx.attrs.programs] + 
        ["}\n"],
    )
    cpp = ctx.actions.declare_output('{}.cpp'.format(ctx.attrs.name))
    cmd = cmd_args(
        ["gpp", '-U', '@', '', '(', ',', ')', '(', ')', '#', '\\', '-I{}'.format(ctx.label.package), gpp, '-o', cpp.as_output()]
    )
    cmd.hidden(ctx.attrs.extra_sources)
    cmd.hidden(['{}.vert'.format(prog) for prog in ctx.attrs.programs])
    cmd.hidden(['{}.frag'.format(prog) for prog in ctx.attrs.programs])
    ctx.actions.run(cmd, category = "gpp")

    return [DefaultInfo(default_output = None, sub_targets = {
        "cpp": [DefaultInfo(default_output = cpp)],
        "hpp": [DefaultInfo(default_output = hpp)]
    })]

shader_data = rule(impl = shader_data_impl, attrs = {
  "programs": attrs.list(attrs.string()),
  "extra_sources": attrs.list(attrs.string(), default = []),
  'header': attrs.string(),
  'namespace': attrs.string(),
})

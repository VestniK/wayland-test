def gen_wayland_protocol_impl(ctx) -> ["provider"]:
    hdr = ctx.actions.declare_output(ctx.attrs.header)
    src = ctx.actions.declare_output(ctx.attrs.source)
    ctx.actions.run(
        ['wayland-scanner', 'client-header', ctx.attrs.spec, hdr.as_output()],
        category = "wl_scan_h"
    )
    ctx.actions.run(
        ['wayland-scanner', 'private-code', ctx.attrs.spec, src.as_output()],
        category = "wl_scan_c"
    )
    return [DefaultInfo(default_output = src, sub_targets = {
        "h": [DefaultInfo(default_output = hdr)]
    })]

gen_wayland_protocol = rule(
    impl = gen_wayland_protocol_impl,
    attrs = {
        "spec": attrs.source(),
        "source": attrs.string(),
        "header": attrs.string(),
    },
)
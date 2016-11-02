#include <mbgl/renderer/painter.hpp>
#include <mbgl/programs/programs.hpp>
#include <mbgl/programs/fill_program.hpp>
#include <mbgl/util/clip_id.hpp>

namespace mbgl {

void Painter::renderClippingMask(const UnwrappedTileID& tileID, const ClipID& clip) {
    programs->fill.draw(
        context,
        gl::Triangles(),
        gl::DepthMode::disabled(),
        gl::StencilMode {
            gl::StencilMode::Always(),
            static_cast<int32_t>(clip.reference.to_ulong()),
            0b11111111,
            gl::StencilMode::Keep,
            gl::StencilMode::Keep,
            gl::StencilMode::Replace
        },
        gl::ColorMode::disabled(),
        FillUniforms::values(
           matrixForTile(tileID),
           0.0f,
           Color {},
           Color {},
           context.viewport.getCurrentValue().size
        ),
        tileTriangleVertexBuffer
    );
}

} // namespace mbgl

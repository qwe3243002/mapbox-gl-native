#include <mbgl/programs/line_program.hpp>
#include <mbgl/style/layers/line_layer_properties.hpp>
#include <mbgl/renderer/render_tile.hpp>
#include <mbgl/map/transform_state.hpp>
#include <mbgl/util/mat2.hpp>
#include <mbgl/sprite/sprite_atlas.hpp>
#include <mbgl/geometry/line_atlas.hpp>

namespace mbgl {

using namespace style;

static_assert(sizeof(LineAttributes::Vertex) == 8, "expected LineVertex size");

template <class Values, class...Args>
Values makeValues(const LinePaintProperties::Evaluated& properties,
                  float pixelRatio,
                  const RenderTile& tile,
                  const TransformState& state,
                  Args&&... args) {
    // the distance over which the line edge fades out.
    // Retina devices need a smaller distance to avoid aliasing.
    float antialiasing = 1.0 / pixelRatio;

    mat2 antialiasingMatrix;
    matrix::identity(antialiasingMatrix);
    matrix::scale(antialiasingMatrix, antialiasingMatrix, 1.0, std::cos(state.getPitch()));
    matrix::rotate(antialiasingMatrix, antialiasingMatrix, state.getAngle());

    // calculate how much longer the real world distance is at the top of the screen
    // than at the middle of the screen.
    float topedgelength = std::sqrt(std::pow(state.getSize().height, 2.0f) / 4.0f  * (1.0f + std::pow(state.getAltitude(), 2.0f)));
    float x = state.getSize().height / 2.0f * std::tan(state.getPitch());

    return Values {
        tile.translatedMatrix(properties.get<LineTranslate>(),
                              properties.get<LineTranslateAnchor>(),
                              state),
        properties.get<LineOpacity>(),
        properties.get<LineWidth>() / 2,
        properties.get<LineGapWidth>() / 2,
        properties.get<LineBlur>() + antialiasing,
        -properties.get<LineOffset>(),
        antialiasing / 2,
        antialiasingMatrix,
        1.0f / tile.id.pixelsToTileUnits(1.0, state.getZoom()),
        (topedgelength + x) / topedgelength - 1.0f,
        std::forward<Args>(args)...
    };
}

LineProgram::UniformValues
LineProgram::uniformValues(const LinePaintProperties::Evaluated& properties,
                           float pixelRatio,
                           const RenderTile& tile,
                           const TransformState& state) {
    return makeValues<LineProgram::UniformValues>(
        properties,
        pixelRatio,
        tile,
        state,
        properties.get<LineColor>()
    );
}

LineSDFProgram::UniformValues
LineSDFProgram::uniformValues(const LinePaintProperties::Evaluated& properties,
                              float pixelRatio,
                              const RenderTile& tile,
                              const TransformState& state,
                              const LinePatternPos& posA,
                              const LinePatternPos& posB,
                              float dashLineWidth,
                              float atlasWidth) {
    const float widthA = posA.width * properties.get<LineDasharray>().fromScale * dashLineWidth;
    const float widthB = posB.width * properties.get<LineDasharray>().toScale * dashLineWidth;

    std::array<float, 2> scaleA {{
        1.0f / tile.id.pixelsToTileUnits(widthA, state.getIntegerZoom()),
        -posA.height / 2.0f
    }};

    std::array<float, 2> scaleB {{
        1.0f / tile.id.pixelsToTileUnits(widthB, state.getIntegerZoom()),
        -posB.height / 2.0f
    }};

    return makeValues<LineSDFProgram::UniformValues>(
        properties,
        pixelRatio,
        tile,
        state,
        properties.get<LineColor>(),
        scaleA,
        scaleB,
        posA.y,
        posB.y,
        properties.get<LineDasharray>().t,
        atlasWidth / (std::min(widthA, widthB) * 256.0f * pixelRatio) / 2.0f,
        0
    );
}

LinePatternProgram::UniformValues
LinePatternProgram::uniformValues(const LinePaintProperties::Evaluated& properties,
                                  float pixelRatio,
                                  const RenderTile& tile,
                                  const TransformState& state,
                                  const SpriteAtlasPosition& posA,
                                  const SpriteAtlasPosition& posB) {
     std::array<float, 2> sizeA {{
         tile.id.pixelsToTileUnits(posA.size[0] * properties.get<LinePattern>().fromScale, state.getIntegerZoom()),
         posA.size[1]
     }};

     std::array<float, 2> sizeB {{
         tile.id.pixelsToTileUnits(posB.size[0] * properties.get<LinePattern>().toScale, state.getIntegerZoom()),
         posB.size[1]
     }};

    return makeValues<LinePatternProgram::UniformValues>(
        properties,
        pixelRatio,
        tile,
        state,
        posA.tl,
        posA.br,
        posB.tl,
        posB.br,
        sizeA,
        sizeB,
        properties.get<LinePattern>().t,
        0
    );
}

} // namespace mbgl

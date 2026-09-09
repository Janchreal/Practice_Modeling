#include "extrusion_recipe.h"

namespace ExtrusionRecipeUtils {

FeatureRecipe buildModelExtrusionRecipe(int modelIndex,
                                        const gp_Dir& direction,
                                        double distance,
                                        bool makeSheetBody,
                                        bool useVectorDirection)
{
    FeatureRecipe recipe;
    recipe.hasRecipe = true;
    recipe.parentIndices = { modelIndex };
    recipe.extrusion.profileModelIndices = { modelIndex };
    recipe.extrusion.direction = direction;
    recipe.extrusion.lengthFwd = distance;
    recipe.extrusion.makeSheetBody = makeSheetBody;
    recipe.extrusion.useVectorDirection = useVectorDirection;
    return recipe;
}

} // namespace ExtrusionRecipeUtils

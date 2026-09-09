#ifndef EXTRUSION_RECIPE_H
#define EXTRUSION_RECIPE_H

#include "platformmath.h"

#include "domain/features/featurerecipe.h"

#include <gp_Dir.hxx>

namespace ExtrusionRecipeUtils {

FeatureRecipe buildModelExtrusionRecipe(int modelIndex,
                                        const gp_Dir& direction,
                                        double distance,
                                        bool makeSheetBody,
                                        bool useVectorDirection);

} // namespace ExtrusionRecipeUtils

#endif // EXTRUSION_RECIPE_H

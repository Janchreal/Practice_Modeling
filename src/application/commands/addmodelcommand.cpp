// addmodelcommand.cpp
#include "addmodelcommand.h"
#include "domain/features/featurerecipe.h"
#include "modeling_command_port.h"

#include <QMap>

AddModelCommand::AddModelCommand(ModelingCommandPort* context,
                                 const TopoDS_Shape& shape,
                                 const QString& name,
                                 ModelType type,
                                 const QColor& color,
                                 double param1,
                                 double param2,
                                 double param3,
                                 const QList<int>& hideSourceIndices,
                                 bool hideSourceOnExecute,
                                 const FeatureRecipe* recipe)
    : Command(context),
      shape_(shape),
      name_(name),
      type_(type),
      color_(color),
      param1_(param1),
      param2_(param2),
      param3_(param3),
      hideSourceIndices_(hideSourceIndices),
      hideSourceOnExecute_(hideSourceOnExecute)
{
    if (recipe) {
        recipe_ = *recipe;
        hasRecipe_ = recipe->hasRecipe;
    }
}

void AddModelCommand::execute()
{
    if (!context_) return;

    sourceWasVisibleBefore_.clear();
    if (hideSourceOnExecute_) {
        for (int sourceIndex : hideSourceIndices_) {
            if (sourceIndex >= 0) {
                sourceWasVisibleBefore_.insert(sourceIndex, context_->isModelVisibleForCommand(sourceIndex));
            }
        }
    }

    context_->displayOccShape(shape_, name_, type_, color_, param1_, param2_, param3_);
    createdIndex_ = context_->getHistorySize() - 1;

    if (hasRecipe_) {
        context_->assignFeatureRecipe(createdIndex_, recipe_);
    }

    if (hideSourceOnExecute_) {
        for (int sourceIndex : hideSourceIndices_) {
            if (sourceIndex >= 0) {
                context_->setModelVisibleForCommand(sourceIndex, false);
            }
        }
    }
}

void AddModelCommand::undo()
{
    if (!context_) return;

    if (createdIndex_ >= 0) {
        context_->removeModelByIndex(createdIndex_);
    }

    if (hideSourceOnExecute_) {
        for (auto it = sourceWasVisibleBefore_.cbegin(); it != sourceWasVisibleBefore_.cend(); ++it) {
            context_->setModelVisibleForCommand(it.key(), it.value());
        }
    }
}

QString AddModelCommand::getDescription() const
{
    // name_ 里一般已包含操作含义（倒圆角/倒角/旋转/挖空...）
    return name_;
}



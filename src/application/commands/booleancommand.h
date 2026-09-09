#ifndef BOOLEANCOMMAND_H
#define BOOLEANCOMMAND_H

#include "command.h"

#include <QList>

class ModelingCommandPort;

class BooleanCommand : public Command {
public:
    BooleanCommand(ModelingCommandPort* context,
                   int targetIndex,
                   const QList<int>& toolIndices,
                   int operationType,
                   bool keepTarget,
                   bool keepTool,
                   const QString& operationName);

    void execute() override;
    void undo() override;
    QString getDescription() const override;

private:
    int targetIndex;
    QList<int> toolIndices;
    int operationType;
    bool keepTarget;
    bool keepTool;
    QString operationName;
    int resultIndex = -1;
};

#endif // BOOLEANCOMMAND_H


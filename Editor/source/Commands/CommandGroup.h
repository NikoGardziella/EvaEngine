#pragma once
#include "Command.h"
#include <memory>
#include <vector>

namespace Engine {

    class CommandGroup : public Command {

    public:
        void AddCommand(std::unique_ptr<Command> command);
        virtual void Execute() override;
        virtual void Undo() override;

        bool IsEmpty() const { return m_Commands.empty(); }

    private:
        std::vector<std::unique_ptr<Command>> m_Commands;
    };

}


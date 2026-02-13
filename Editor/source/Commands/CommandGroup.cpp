#include "CommandGroup.h"

namespace Engine {


    void CommandGroup::AddCommand(std::unique_ptr<Command> command) {
        m_Commands.push_back(std::move(command));
    }

    void CommandGroup::Execute()
    {
        for (auto& cmd : m_Commands) cmd->Execute();
    }

    void CommandGroup::Undo() 
    {
        // Undo in reverse order to maintain stack integrity
        for (auto it = m_Commands.rbegin(); it != m_Commands.rend(); ++it)       
        {
            (*it)->Undo();
        }
    }

}


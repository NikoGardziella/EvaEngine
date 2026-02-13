#pragma once
#include "Command.h"
#include <deque>
#include <memory>
#include <vector>

namespace Engine {

    class CommandHistory {
    public:
        void Push(std::unique_ptr<Command> command)
        {
            command->Execute();
            m_UndoStack.push_back(std::move(command));

            // Limit history size to 50 actions
            if (m_UndoStack.size() > 50) m_UndoStack.pop_front();

            m_RedoStack.clear();
        }

        void Undo() {
            if (m_UndoStack.empty()) return;

            auto cmd = std::move(m_UndoStack.back());
            m_UndoStack.pop_back();
            cmd->Undo();
            m_RedoStack.push_back(std::move(cmd));
        }

        void Redo() {
            if (m_RedoStack.empty()) return;

            auto cmd = std::move(m_RedoStack.back());
            m_RedoStack.pop_back();
            cmd->Execute();
            m_UndoStack.push_back(std::move(cmd));
        }

    private:
        std::deque<std::unique_ptr<Command>> m_UndoStack;
        std::vector<std::unique_ptr<Command>> m_RedoStack;
    };

}
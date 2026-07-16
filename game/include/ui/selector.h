#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "engine/engine.h"
#include "engine/utils/path.h"
#include "engine/utils/logger.h"

class Game;

class Selector {
public:
    Selector(Game& game, std::string m_searchPath, std::string m_fileExtension)
        : m_game(game), m_searchPath(std::move(m_searchPath)), m_fileExtension(std::move(m_fileExtension)) {};

    void Draw() {
        Vec2 windowSize = m_game.GetEngine().getWindow().getSize();
        ImGui::SetNextWindowPos(ImVec2(windowSize.x * 0.5f - 150, windowSize.y * 0.5f - 100), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);
        ImGui::Begin(m_game.GetUI().getText("selector.open"), &m_isOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        std::filesystem::path resolvedPath = Path::resolve(m_searchPath);

        if (!std::filesystem::exists(resolvedPath)) {
            ImGui::Text(m_game.GetUI().getText("selector.nsfod"));
            ImGui::Text("%s", m_searchPath.c_str());
            ImGui::End();
            return;
        }

        std::vector<std::string> files;
        for (const auto& entry : std::filesystem::directory_iterator(resolvedPath)) {
            if (entry.is_regular_file()) {
                const auto& path = entry.path();
                if (path.extension().generic_string() == m_fileExtension) {
                    files.push_back(path.filename().string());
                }
            }
        }

        std::string fileToDelete;

        for (const auto& file : files) {
            ImGui::PushID(file.c_str());

            float deleteButtonWidth = ImGui::CalcTextSize("X").x + ImGui::GetStyle().FramePadding.x * 2.0f;
            float availableWidth = ImGui::GetContentRegionAvail().x - deleteButtonWidth - ImGui::GetStyle().ItemSpacing.x;

            if (ImGui::Selectable(file.c_str(), false, 0, ImVec2(availableWidth, 0))) {
                std::filesystem::path fullPath = resolvedPath / file;
                m_selectedPath = fullPath.string();
                m_isOpen = false;
            }

            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.1f, 0.1f, 1.0f));
            if (m_game.GetUI().button("X", ImVec2(deleteButtonWidth, 0))) {
                fileToDelete = file;
            }
            ImGui::PopStyleColor();

            ImGui::PopID();
        }

        ImGui::End();

        if (!fileToDelete.empty()) {
            std::filesystem::path fullPath = resolvedPath / fileToDelete;
            std::error_code ec;
            std::filesystem::remove(fullPath, ec);
            if (ec) {
                Logger::error("Failed to delete file: {}", ec.message());
            }
        }
    }

    std::string ConsumeSelection() {
        std::string temp = m_selectedPath;
        m_selectedPath.clear();
        return temp;
    }

    void toggleOpen() { m_isOpen = !m_isOpen; }
    void setOpen(bool open) { m_isOpen = open; }
    bool isOpen() const { return m_isOpen; }
private:
    Game& m_game;

    bool m_isOpen = false;

    std::string m_searchPath;
    std::string m_fileExtension;
    std::string m_selectedPath;
};
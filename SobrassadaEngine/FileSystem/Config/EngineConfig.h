#pragma once

#include "ConfigFile.h"
#include "HashString.h"
#include <string>
#include <unordered_set>

class EngineConfig : public ConfigFile
{
  public:
    EngineConfig();
    ~EngineConfig() override;

    void Load() override;
    void Save() const override;

    void ClearPreviouslyLoadedProjectPaths();

    const std::string& GetStartupProjectPath() const { return startupProjectPath; }
    const std::unordered_set<HashString>& GetProjectPaths() const { return previouslyLoadedProjectPaths; }

    bool ShouldStartGameOnStartup() const { return startGameOnStartup; }

    void SetStartupProjectPath(const std::string& newStartupProjectPath);

  private:
    std::string startupProjectPath = "";
    std::unordered_set<HashString> previouslyLoadedProjectPaths;
    bool startGameOnStartup;
};

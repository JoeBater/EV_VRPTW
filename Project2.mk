##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=Project2
ConfigurationName      :=Debug
WorkspaceConfiguration :=Debug
WorkspacePath          :=C:/Users/user/Workspaces/Workspaces
ProjectPath            :=C:/Users/user/Workspaces/Workspaces/Project2
IntermediateDirectory  :=../build-$(WorkspaceConfiguration)/Project2
OutDir                 :=$(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=user
Date                   :=01/02/2026
CodeLitePath           :="C:/Program Files/CodeLite"
MakeDirCommand         :=mkdir
LinkerName             :=C:/mingw64/bin/g++.exe
SharedObjectLinkerName :=C:/mingw64/bin/g++.exe -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputDirectory        :=C:/Users/user/Workspaces/Workspaces/build-$(WorkspaceConfiguration)/bin
OutputFile             :=..\build-$(WorkspaceConfiguration)\bin\$(ProjectName).exe
Preprocessors          :=
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :=$(IntermediateDirectory)/ObjectsList.txt
PCHCompileFlags        :=
RcCmpOptions           := 
RcCompilerName         :=C:/mingw64/bin/windres.exe
LinkOptions            :=  -static
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). 
IncludePCH             := 
RcIncludePath          := 
Libs                   := 
ArLibs                 :=  
LibPath                := $(LibraryPathSwitch). 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overridden using an environment variable
##
AR       := C:/mingw64/bin/ar.exe -r
CXX      := C:/mingw64/bin/g++.exe
CC       := C:/mingw64/bin/gcc.exe
CXXFLAGS :=  -gdwarf-2 -O0 -Wall $(Preprocessors)
CFLAGS   :=  -O0 -gdwarf-2 -Wall $(Preprocessors)
ASFLAGS  := 
AS       := C:/mingw64/bin/as.exe


##
## User defined environment variables
##
CodeLiteDir:=C:\Program Files\CodeLite
Objects0=$(IntermediateDirectory)/Route.cpp$(ObjectSuffix) $(IntermediateDirectory)/LNS.cpp$(ObjectSuffix) $(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IntermediateDirectory)/exp0.cpp$(ObjectSuffix) $(IntermediateDirectory)/exp2.cpp$(ObjectSuffix) $(IntermediateDirectory)/Instance.cpp$(ObjectSuffix) $(IntermediateDirectory)/exp1.cpp$(ObjectSuffix) $(IntermediateDirectory)/SubStep.cpp$(ObjectSuffix) $(IntermediateDirectory)/LNSar.cpp$(ObjectSuffix) $(IntermediateDirectory)/MST.cpp$(ObjectSuffix) \
	



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: MakeIntermediateDirs $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@if not exist "$(IntermediateDirectory)" $(MakeDirCommand) "$(IntermediateDirectory)"
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

MakeIntermediateDirs:
	@if not exist "$(IntermediateDirectory)" $(MakeDirCommand) "$(IntermediateDirectory)"
	@if not exist "$(OutputDirectory)" $(MakeDirCommand) "$(OutputDirectory)"

$(IntermediateDirectory)/.d:
	@if not exist "$(IntermediateDirectory)" $(MakeDirCommand) "$(IntermediateDirectory)"

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/Route.cpp$(ObjectSuffix): Route.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/user/Workspaces/Workspaces/Project2/Route.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Route.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Route.cpp$(PreprocessSuffix): Route.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Route.cpp$(PreprocessSuffix) Route.cpp

$(IntermediateDirectory)/LNS.cpp$(ObjectSuffix): LNS.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/user/Workspaces/Workspaces/Project2/LNS.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/LNS.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/LNS.cpp$(PreprocessSuffix): LNS.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/LNS.cpp$(PreprocessSuffix) LNS.cpp

$(IntermediateDirectory)/main.cpp$(ObjectSuffix): main.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/user/Workspaces/Workspaces/Project2/main.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/main.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/main.cpp$(PreprocessSuffix): main.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/main.cpp$(PreprocessSuffix) main.cpp

$(IntermediateDirectory)/exp0.cpp$(ObjectSuffix): exp0.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/user/Workspaces/Workspaces/Project2/exp0.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/exp0.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/exp0.cpp$(PreprocessSuffix): exp0.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/exp0.cpp$(PreprocessSuffix) exp0.cpp

$(IntermediateDirectory)/exp2.cpp$(ObjectSuffix): exp2.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/user/Workspaces/Workspaces/Project2/exp2.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/exp2.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/exp2.cpp$(PreprocessSuffix): exp2.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/exp2.cpp$(PreprocessSuffix) exp2.cpp

$(IntermediateDirectory)/Instance.cpp$(ObjectSuffix): Instance.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/user/Workspaces/Workspaces/Project2/Instance.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/Instance.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/Instance.cpp$(PreprocessSuffix): Instance.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/Instance.cpp$(PreprocessSuffix) Instance.cpp

$(IntermediateDirectory)/exp1.cpp$(ObjectSuffix): exp1.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/user/Workspaces/Workspaces/Project2/exp1.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/exp1.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/exp1.cpp$(PreprocessSuffix): exp1.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/exp1.cpp$(PreprocessSuffix) exp1.cpp

$(IntermediateDirectory)/SubStep.cpp$(ObjectSuffix): SubStep.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/user/Workspaces/Workspaces/Project2/SubStep.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/SubStep.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/SubStep.cpp$(PreprocessSuffix): SubStep.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/SubStep.cpp$(PreprocessSuffix) SubStep.cpp

$(IntermediateDirectory)/LNSar.cpp$(ObjectSuffix): LNSar.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/user/Workspaces/Workspaces/Project2/LNSar.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/LNSar.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/LNSar.cpp$(PreprocessSuffix): LNSar.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/LNSar.cpp$(PreprocessSuffix) LNSar.cpp

$(IntermediateDirectory)/MST.cpp$(ObjectSuffix): MST.cpp 
	$(CXX) $(IncludePCH) $(SourceSwitch) "C:/Users/user/Workspaces/Workspaces/Project2/MST.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/MST.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/MST.cpp$(PreprocessSuffix): MST.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/MST.cpp$(PreprocessSuffix) MST.cpp

##
## Clean
##
clean:
	$(RM) -r $(IntermediateDirectory)



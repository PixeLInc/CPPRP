::python3 "./scripts/preprocessgameclass.py" "./CPPRP/data/GameClasses.h" json > "./CPPRP/generated/GameClassMacros.h"
::python3 "./scripts/preprocessgameclass.py" "./CPPRP/data/GameClasses.h" classext > "./CPPRP/generated/ClassExtensions.h"
::python3 "./scripts/preprocessnetworkdata.py" "./CPPRP/data/NetworkData.h" consume > "./CPPRP/generated/NetworkDataParsersGenerated.h"
::python3 "./scripts/preprocessnetworkdata.py" "./CPPRP/data/NetworkData.h" json > "./CPPRPJSON/GeneratedSerializeFunctions.h"
MSBuild.exe CPPRP.sln -p:Configuration=JSONBuild -p:Platform=x64

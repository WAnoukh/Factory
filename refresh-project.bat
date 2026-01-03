cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1
::xcopy /f /y .\build\compile_commands.json .\game\exe\compile_commands.json
::xcopy /f /y .\build\compile_commands.json .\game\hot\compile_commands.json

** Console system:
Console is a debug service used to process user commands and log output. User plugins can create custom console
commands. Console command structures contain a delegate for the command, number of arguments, description, usage, and an
optional group. Arguments of a console command are parsed as Json objects. The delegate is responcible for further
de-serialization if it is needed.

If a console command cannot be executed (i.e. due to an invalid argument), the delegate should return an error code
indicating an argument error (via `expected<void, std::error_code>`). Index of the invalid argument is OR'ed with the
error code.

Example:

```cpp

auto console = sek::console::instance(); // Get the main console
auto cmd = console->commands();

sek::console::command cmd1 = 
{
	// Command delegate
	[]() -> expected<void, std::error_code> { sek::logger::info()->log("cmd1"); return {}; },

	0,			// Number of arguments
	"Print \"cmd1\"",	// Description
	"cmd1",			// Usage
	"TEST"	 		// Group
};
sek::console::command print_num = 
{
	[](const sek::json_object &arg) -> expected<void, std::error_code>
	{
		if (!arg.is_number()) // Verify argument type
			return unexpected{make_error_code(sek::console_errc::INVALID_ARG | 0)};

		sek::logger::info()->log(fmt::format("{}", arg.get<double>()));
		return {};
	},
	1,						// Number of arguments
	"Print a number",				// Description
	"printnum [number]",				// Usage
	"TEST"						// Group
};

cmd->try_insert("cmd1", cmd1);		// Insert cmd1 as "cmd1", if it does not exist yet
cmd->insert("printnum", print_num); 	// Insert or overwrite print_num as "printnum"
cmd->erase("cmd2");			// Remove command "cmd2"

```
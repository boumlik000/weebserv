#include "server.hpp"     // Includes everything needed via server.hpp -> ConfigFile.hpp etc.
#include "ConfigFile.hpp" // Make sure to include this for the Webserv class
// #include "webserv.hpp" // Make sure to include this for the Webserv class

int main(int ac, char **av)
{
    // 1. First, check the command-line arguments.
    if (ac != 2)
    {
        // If the number of arguments is not correct, print a usage message and exit.
        std::cerr << "Usage: " << av[0] << " <config_file.conf>" << std::endl;
        return (1); // Exit with an error code.
    }

    try
    {
        // 2. Create a Webserv object to handle the configuration.
        Webserv webserv;

        // 3. Parse the configuration file.
        // The pars_cfile method will handle errors and print messages.
        webserv.start_event(ac, av);

        // 4. Create and run the server using the parsed configuration.
        // We get the configuration from the Webserv object and pass it to the Server.
        Server my_server(webserv.getConfig()); // You'll need to add a simple getter in your Webserv class for this.
        
        // The Server constructor now calls run(), which sets up and starts the event loop.
        // The program will now run inside my_server.run() until it's terminated.
    }
    catch (const std::exception &e)
    {
        // Catch any other unexpected errors that might occur.
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return (1);
    }
    
    return (0); // The program should ideally never reach here as it's in an infinite loop.
}
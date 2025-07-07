#include <Zipper/Zipper.hpp>
#include <sstream>

int main()
{
    zipper::Zipper zipper("test.zip");

    std::stringstream content("Hello World!");
    zipper.add(content, "hello.txt");

    zipper.close();
    return EXIT_SUCCESS;
}
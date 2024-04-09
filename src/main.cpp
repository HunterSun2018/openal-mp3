#include <iostream>
#include <fstream>
#include <iterator>
#include <format>
#include "player.hpp"

using namespace std;

int main(int argc, char const *argv[])
{
    /* code */

    if (argc <= 2)
    {
        cout << "MP3 player by OpenAL.\n"
             << "usage: mp3-player file"
             << endl;
    }

    try
    {
        ifstream ifs(argv[1], ios::binary);

        if (ifs.fail())
            throw runtime_error(format("cannot open file {}", argv[1]));

        // istreambuf_iterator<char> isb(ifs), empty;
        // vector<uint8_t> buffer(isb, empty);

        ifs.seekg(0, std::ifstream::end);
        streampos size = ifs.tellg();
        ifs.seekg(0);

        vector<uint8_t> buffer(size);

        ifs.read((char*)buffer.data(), size);

        auto player = create_play_stream();

        player->OpenStream();

        player->Play(argv[1]);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
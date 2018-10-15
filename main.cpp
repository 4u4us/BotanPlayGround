#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include <botan/rng.h>
#include <botan/auto_rng.h>

//#if defined(BOTAN_HAS_OPENSSL)
//   #include <botan/internal/openssl.h>
//#endif

int main(int argc, char* argv[])
    {
    std::cout << "Botan Test App" << std::endl;
	int rnd_array_len = 10;
	uint8_t rnd_array[rnd_array_len];
	
	std::unique_ptr<Botan::AutoSeeded_RNG> rng_a(new Botan::AutoSeeded_RNG);

	rng_a->randomize( rnd_array, (size_t) rnd_array_len );
	
	std::cout << "-- Using randomize: " << std::endl;
	for(int i=0;i<rnd_array_len;i++)
	{
		std::cout <<  " rnd_array[" << i << "] = 0x" << std::hex << static_cast<int>(rnd_array[i]) << std::endl;
	}
   	return 2;
   	}

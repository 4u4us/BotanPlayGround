#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/p11_x509.h>
#include <botan/x509_dn.h>
#include <botan/der_enc.h>

//#if defined(BOTAN_HAS_OPENSSL)
//   #include <botan/internal/openssl.h>
//#endif
using namespace Botan;
using namespace PKCS11;

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
	std::cout << std::dec;
    
    Botan::PKCS11::Module aModule( "/usr/local/lib/softhsm/libsofthsm2.so");
    //Botan::PKCS11::Module aModule( "/usr/local/lib/libbotan-2.so");
    aModule.reload();
    
    Botan::PKCS11::Info info = aModule.get_info();

    // print library version
    std::cout << "Module HSM version" << std::to_string( info.libraryVersion.major ) << "."
              << std::to_string( info.libraryVersion.minor ) << std::endl;
    // only slots with connected token
    std::vector<Botan::PKCS11::SlotId> slots = Botan::PKCS11::Slot::get_available_slots( aModule, true );

    // use first slot
    Botan::PKCS11::Slot slot( aModule, slots.at( 0 ) );
    std::cout << "-- Using Slot: 0x" << std::hex << slots.at( 0 ) << std::endl;
    std::cout << std::dec;
    
    Botan::PKCS11::Session aSession( slot, false ); // R/W session
    // get session info
    Botan::PKCS11::SessionInfo infosess = aSession.get_info();
    std::cout << "Session slot id : 0x"<< std::hex << infosess.slotID << std::endl;
    std::cout << std::dec;
    // log in as security officer
    Botan::PKCS11::secure_string so_pin = { '1', '2', '3', '4'};
    aSession.login( Botan::PKCS11::UserType::SO, so_pin );
    
    std::cout << "-- Importing to a token a PKCS11 X509: " << std::endl;
    //Botan::X509_Certificate root("../../botan/src/tests/data/x509/nist/root.crt");
    Botan::X509_Certificate root("../../botan/src/tests/data/x509/ecc/root2_SHA256.cer");
    
    // set props
    Botan::PKCS11::X509_CertificateProperties props(
        Botan::DER_Encoder().encode(root.subject_dn()).get_contents_unlocked(), root.BER_encode() );

    props.set_label( "Botan PKCS#11 test certificate" );
    props.set_private( false );
    props.set_token( true ); // not transient, i.e permanent after logoff

    // import TO a token
    Botan::PKCS11::PKCS11_X509_Certificate pkcs11_cert( aSession, props );
    std::cout << "--    Imported handle: " << pkcs11_cert.handle()  << std::endl;
    // load by handle
    Botan::PKCS11::PKCS11_X509_Certificate pkcs11_cert2( aSession, pkcs11_cert.handle() );
    std::cout << "--    Loaded handle: " << pkcs11_cert2.handle()  << std::endl;
    
    // logoff
    aSession.logoff();
    
    aSession.login( Botan::PKCS11::UserType::SO, so_pin );
    // search created object
    std::cout << "-- Searching for this imported PKCS11 X509: " << std::endl;
    AttributeContainer search_template;
    search_template.add_string(AttributeType::Label, "Botan PKCS#11 test certificate");
    search_template.add_binary(AttributeType::Subject, Botan::DER_Encoder().encode(root.subject_dn()).get_contents_unlocked());
    search_template.add_binary(AttributeType::Value, root.BER_encode());
    ObjectFinder finder(aSession, search_template.attributes());

    auto search_result = finder.find();
    std::cout << "--       Item(s) found: " << search_result.size() << std::endl; 
    finder.finish();
    Botan::PKCS11::PKCS11_X509_Certificate pkcs11_cert3( aSession, search_result.at(0) );
    std::cout << "--          Loaded handle: " << pkcs11_cert3.handle()  << std::endl;
        
    aSession.logoff();
    
    std::cout << "-- root.to_string() --" << std::endl;
    std::cout << root.to_string();
    std::cout << "----------------------" << std::endl;
    
    std::cout << "-- pkcs11_cert3.to_string() --" << std::endl;
    std::cout << pkcs11_cert3.to_string();
    std::cout << "----------------------" << std::endl;
    
    // Re-init the token with e.g command line:
    // softhsm2-util --init-token --token "Token-1" --label "Token-1"
    
   	return 2;
   	}

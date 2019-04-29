#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <botan/rng.h>
#include <botan/auto_rng.h>
#include <botan/p11_x509.h>
#include <botan/p11_ecdh.h>
#include <botan/x509_dn.h>
#include <botan/der_enc.h>


//#if defined(BOTAN_HAS_OPENSSL)
//   #include <botan/internal/openssl.h>
//#endif
using namespace Botan;
using namespace PKCS11;

//_____________________________________________________________________________________
using testing::Return; // for gmock
class X509_Certificate_MOCK : public X509_Certificate
   {
    public:
        //X509_Certificate_MOCK(const std::string& fname) : X509_Certificate(fname) {};
        X509_Certificate_MOCK(const std::string& fname){};
        MOCK_METHOD0(x509_version, uint32_t(void));

   };
//_____________________________________________________________________________________

int main(int argc, char* argv[])
    {
    // Re-init the token with e.g command line:
    // softhsm2-util --init-token --token "Token-1" --label "Token-1"
    
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
    //Botan::X509_Certificate root("../../BOTAN/botan/src/tests/data/x509/ecc/root2_SHA256.cer");
    Botan::X509_Certificate root("/home/eric/BOTAN/botan/src/tests/data/x509/ecc/root2_SHA256.cer");
    
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
        
    std::cout << "-- root.to_string() --" << std::endl;
    std::cout << root.to_string() << std::endl;
    std::cout << "----------------------" << std::endl;
    
    std::cout << "-- pkcs11_cert3.to_string() --" << std::endl;
    std::cout << pkcs11_cert3.to_string() << std::endl;
    std::cout << "----------------------" << std::endl;
    
    pkcs11_cert.destroy();
    //pkcs11_cert2.destroy(); // pkcs11_cert2 was created using the handle of pkcs11_cert
    //pkcs11_cert3.destroy(); // pkcs11_cert3 was created using the handle from a search to find pkcs11_cert
    aSession.logoff();

    std::cout << "-- Import ECDH private key import --" << std::endl; 
    aSession.login( Botan::PKCS11::UserType::User, so_pin ); // login as user for key import
    // create ecdh private key
    ECDH_PrivateKey priv_key(*rng_a, EC_Group("secp256r1"));
    priv_key.set_parameter_encoding(EC_Group_Encoding::EC_DOMPAR_ENC_OID);
    // import to card
    EC_PrivateKeyImportProperties propsk(priv_key.DER_domain(), priv_key.private_value());
    propsk.set_token(true);
    propsk.set_private(true);
    propsk.set_derive(true);
    propsk.set_extractable(true);
    // label
    std::string label = "Botan test ecdh key";
    propsk.set_label(label);
    PKCS11_ECDH_PrivateKey pk(aSession, propsk);
    Object pk_a(aSession, pk.handle());
    secure_vector<uint8_t> key_label = pk_a.get_attribute_value(AttributeType::Label);
    std::string key_label_str(key_label.begin(),key_label.end());
    std::cout << key_label_str << std::endl; 
    std::cout << "-- ECDH private key import was successful --" << std::endl; 
    pk.destroy();
    aSession.logoff();
    
    std::cout << "-- X509_Certificate root.x509_version() --" << std::endl;
    std::cout << root.x509_version() << std::endl;
    std::cout << "----------------------" << std::endl;
    // Test with gmock
    testing::InitGoogleMock(&argc,argv);
    X509_Certificate_MOCK root_MOCK("../../BOTAN/botan/src/tests/data/x509/nist/root.crt");
    EXPECT_CALL(root_MOCK, x509_version())
        .WillOnce(Return(666));
    uint32_t root_x509_version_MOCK = root_MOCK.x509_version();
    std::cout << "-- X509_Certificate_MOCK root_MOCK.x509_version() --" << std::endl;
    std::cout << root_x509_version_MOCK << std::endl;
    std::cout << "----------------------" << std::endl;

        
   	return 2;
   	}

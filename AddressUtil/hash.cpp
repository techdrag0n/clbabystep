#include "AddressUtil.h"
#include "CryptoUtil.h"
#include <stdio.h>
#include <string.h>
/*
static unsigned int endian(unsigned int x)
{
	return (x << 24) | ((x << 8) & 0x00ff0000) | ((x >> 8) & 0x0000ff00) | (x >> 24);
}
*/
bool Address::verifyAddress(std::string address)
{
	// Check length
	if (address.length() == 128) {

	}else{
	if(address.length() > 34) {
		false;
	}
}
	// Check encoding
	if(!Base58::isBase58(address)) {
		return false;
	}

	std::string noPrefix = address.substr(1);

	secp256k1::uint256 value = Base58::toBigInt(noPrefix);
	unsigned int words[8];
	unsigned int hash[8];
	unsigned int checksum;

	value.exportWords(words, 6, secp256k1::uint256::BigEndian);
	memcpy(hash, words, sizeof(unsigned int) * 5);
	checksum = words[5];

	return crypto::checksum(hash) == checksum;
}


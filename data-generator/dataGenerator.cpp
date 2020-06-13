#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <stdio.h>
#include <string>
#include <stdlib.h>

typedef uint64_t Timestamp;
using NanoSeconds = std::chrono::nanoseconds;
using Clock = std::chrono::high_resolution_clock;
const int READ_FILE_BUFFERSIZE = 4056; // 52 tupels

struct __attribute__((packed)) bit {
	uint64_t auction;
	uint64_t bidder;
	uint64_t price;
  	uint64_t dateTime;
  bit() {
    
  }


}; // size 78 bytes




int main(int argc, char *argv[]) {
  // Generator Code
  if (argc != 4) {
    std::cout << "1. argument: Number of tuples. 2. argument: Number of persons. 3 argument: Number of auctions"
              << std::endl;
    return -1;
  }
  if (atoi(argv[1]) % 10 != 0) {
    std::cout << "Number of tuples to be generated should be divisible by 10."
              << std::endl;
    return -1;
  }
  size_t auctionCnt = atoi(argv[3]);
  size_t personCnt = atoi(argv[2]);
  size_t processCnt = atoi(argv[1]);

  bit *recs = new bit[processCnt];

  for (size_t i = 0; i < processCnt; i++) {
	  recs[i].auction = rand() % auctionCnt;
	  recs[i].bidder = rand() % personCnt;
	  recs[i].price = rand() % 100;
	  recs[i].dateTime = i;
  }

  //printGenerated(10, recs);

  std::ofstream ofp("nexmark_test_data.bin", std::ios::out | std::ios::binary);
  ofp.write(reinterpret_cast<const char *>(recs), processCnt * sizeof(bit));
  ofp.close();
}

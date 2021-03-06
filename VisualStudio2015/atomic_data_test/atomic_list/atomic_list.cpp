/*

A linked list using atomic_data. A certain number of elements are pre-inserted into the list.
Then threads perform equal number of insertions and deletions.
If the implementation is correct than at the end the list should be the same size.

For lock-free lists we have to deal with the deletion problem. For this we employ a lock
on the to be deleted node. For testing purposes we set the lock on one of the preinserted elements.
At the end we print out the list and expect that element to remain in the list.

License: Public-domain Software.

Blog post: http://alexpolt.github.io/atomic-data.html
Alexandr Poltavsky

*/


#include <cstdio>
#include <thread>
#include <random>
#include <chrono>

#include "atomic_list.h"


using uint = unsigned;
const uint threads_size = 16;
const uint iterations = 32768;
const uint list_size = 13;

template< typename T > void print_list( T& );

int main() {

  printf( "Test parameters:\n\t CPU: %d core(s)\n\t list size: %d\n\t iterations/thread: %d\n\t threads: %d\n\n",
    std::thread::hardware_concurrency(), list_size, iterations, threads_size );

  printf( "start testing atomic_list<int>\n\n" );

  using atomic_list_t = atomic_list<uint, threads_size*2>;

  //create an instance of atomic_list
  atomic_list_t atomic_list0;

  //used for generating values for insertion
  std::atomic<uint> counter{ 1 };


  //populate the list with list_size members
  //after test insertions/removals we will check that the size is still list_size
  for( uint i = 0; i < list_size; i++ ) {
    atomic_list_t::iterator it = atomic_list0.push_front( counter.fetch_add( 1, std::memory_order_relaxed ) );
  }

  (++atomic_list0.begin()).update( -1 );

  printf( "list before test:\n" );
  print_list( atomic_list0 );

  //insertions
  auto fn_insert = [ &atomic_list0, &counter ]() {

    //generate random position
    std::uniform_int_distribution<uint> engine0{ 0, list_size*2 };
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937_64 gen0( seed );

    for( uint i = 0; i < iterations; i++ ){

      uint value = counter.fetch_add( 1, std::memory_order_relaxed );

      //do insert in a loop because we might try to insert at a locked node
      do {
        uint index = engine0( gen0 );

        auto it = atomic_list0.begin();
        auto it_next = it;

        for( uint i = 0; i < index && it_next; it = it_next++, ++i );

        auto r = atomic_list0.insert_after_weak( it, value );

        if( r ) {
          //printf( "insert at %d value %d\n", index, value );
          break;
        }

      } while( true );

      std::this_thread::yield();
    }
  };


  //deletions
  auto fn_remove = [ &atomic_list0 ]() {

    //generate random position
    std::uniform_int_distribution<uint> engine0{ 0, list_size*2 };
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937_64 gen0( seed );

    for( uint i = 0; i < iterations; i++ ){

      //do insert in a loop because we might try to remove a locked node
      do {
        uint index = engine0( gen0 );

        auto it = atomic_list0.begin();
        auto it_next = it;

        for( uint i = 0; i < index && it_next; it = it_next++, ++i );

        auto r = atomic_list0.erase_after_weak( it );

        if( r ) {
          //printf( "remove at %d value %d\n", index, (*r)->data );
          break;
        }

      } while( true );

      std::this_thread::yield();
    }
  };


  printf( "\nstarting %u threads\n\n", threads_size );

  std::thread threads[ threads_size ];

  for( uint i = 0; i < threads_size; i++ ) 
    threads[ i ] = i % 2 == 0 ? std::thread{ fn_remove } : std::thread{ fn_insert };

  for( auto& thread : threads ) thread.join();


  printf( "list after test:\n");
  print_list( atomic_list0 );

  printf( "\ntest: %s!\n\n", list_size == atomic_list0.size() ? "Passed" : "Failed" );

  printf( "clear atomic_list " );
  atomic_list0.clear();
  printf( "= *%d* elements left\n", atomic_list0.size() );

  printf("\npress enter\n");
  getchar();

}

template< typename T > void print_list( T& atomic_list ) {
  for( auto value : atomic_list ) {
      printf( "%d ", value );
  }
  printf( "= *%d* elements\n", atomic_list.size() );
}


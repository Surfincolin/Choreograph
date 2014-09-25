
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "choreograph/Choreograph.hpp"
#include "cinder/Timer.h"
#include "cinder/Vector.h"
#include "cinder/Timeline.h"

using namespace std;
using namespace cinder;
using namespace choreograph;

class ScopedTimer
{
public:
  ScopedTimer( const std::string &message, float *output = nullptr ):
  _message( message ),
  _output( output )
  {}
  ~ScopedTimer()
  {
    _timer.stop();
    string message = "[" + _message + "]";
    if( message.size() < 50 ) {
      message = message + string( 50 - message.size(), ' ' );
    }
    cout << message << _timer.getSeconds() * 1000 << "ms" << endl;
    if( _output ) {
      *_output = _timer.getSeconds() * 1000;
    }
  }
private:
  float       *_output = nullptr;
  std::string _message;
  ci::Timer   _timer = ci::Timer( true );
};

TEST_CASE( "Creating Motions" ) {
  co::Timeline test_timeline;
  float dt = 1.0f / 60.0f;
  const size_t count = 150e3; // 10k
  SequenceRef<vec2> sequence = make_shared<Sequence<vec2>>( vec2( 1.0f ) );
  sequence->rampTo( vec2( 5.0f ), 1.0f ).rampTo( vec2( 10.0f, 6.0f ), 0.5f );

  cout << "Working with " << count << " motions." << endl;

  vector<Output<vec2>> targets( count, vec2( 0 ) );
  {
    ScopedTimer timer( "Creating Motions" );
    for( auto &target : targets ) {
      test_timeline.move( &target ).getSequence().rampTo( vec2( 10.0f ), 1.0f );
    }
  }

  {
    ScopedTimer timer( "Stepping Motions 1x" );

    test_timeline.step( dt );
  }

  {
    ScopedTimer timer( "Stepping Motions 60x" );
    for( int i = 0; i < 60; ++i ) {
      test_timeline.step( dt );
    }
  }

  {
    ScopedTimer timer( "Clearing Motions" );
    for( auto &target : targets ) {
      target.disconnect();
    }
  }

  {
    ScopedTimer timer( "Stepping Empty Timeline 60x" );
    for( int i = 0; i < 60; ++i ) {
      test_timeline.step( dt );
    }
  }

  {
    ScopedTimer timer( "Creating Motions from shared Sequence" );
    for( auto &target : targets ) {
      test_timeline.move( &target, sequence );
    }
  }

  {
    ScopedTimer timer( "Stepping Motions with shared Sequence 1x" );

    test_timeline.step( dt );
  }

  {
    ScopedTimer timer( "Stepping Motions with shared Sequence 60x" );
    for( int i = 0; i < 60; ++i ) {
      test_timeline.step( dt );
    }
  }
}

TEST_CASE( "Comparative Performance with ci::Timeline", "[library]" ) {
  co::Timeline test_timeline;
  ci::TimelineRef cinder_timeline = ci::Timeline::create();

  const int tween_count = 5000;
  const float dt = 1.0f / 60.0f;

  SECTION( "Tween many targets" ) {
    cout << "Many Targets, One Phrase Each." << endl;
    for( int j = 0; j < 2; ++j )
    {
      vector<Output<vec2>> targets( tween_count, vec2( 0 ) );
      vector<Anim<vec2>> cinder_targets( tween_count, vec2( 0 ) );
      float choreograph, cinder;

      { // Measure basic performance.
        ScopedTimer performance( "Choreograph Timeline", &choreograph );
        //
        for( int i = 0; i < tween_count; ++i )
        {
          test_timeline.move( &targets[i] ).getSequence().hold( 1.0f ).rampTo( vec2( i * 5, i * 20 ), 2.0f );
        }

        ScopedTimer steps( "Choreograph Steps" );
        for( float t = 0.0f; t <= 3.0f; t += dt ) {
          test_timeline.step( dt );
        }
      }

      { // Compare to cinder timeline perf.
        ScopedTimer perf( "Cinder Timeline", &cinder );
        for( int i = 0; i < tween_count; ++i )
        {
          cinder_timeline->apply( &cinder_targets[i], vec2( i * 5, i * 20 ), 2.0f ).delay( 1.0f );
        }

        ScopedTimer steps( "Cinder Steps" );
        for( float t = 0.0f; t <= 3.0f; t += dt ) {
          cinder_timeline->step( dt );
        }
      }

      cout << "Comparative timeline performance (Choreograph:Cinder): " << (choreograph / cinder) << endl;
    }

  }

  SECTION( "Tween one target many times" ) {
    cout << "Single Target, Many Phrases." << endl;
    for( int j = 0; j < 2; ++j )
    {
      Output<vec2>  target( vec2( 0 ) );
      Anim<vec2>    cinder_target( vec2( 0 ) );
      float choreograph, cinder;

      { // Measure basic performance.
        ScopedTimer performance( "Choreograph Timeline", &choreograph );
        //
        for( int i = 0; i < tween_count; ++i )
        {
          test_timeline.queue( &target ).hold( 1.0f ).rampTo( vec2( i * 5, i * 20 ), 2.0f );
        }

        ScopedTimer steps( "Choreograph Steps" );
        for( float t = 0.0f; t <= 3.0f; t += dt ) {
          test_timeline.step( dt );
        }
      }

      { // Compare to cinder timeline perf.
        ScopedTimer perf( "Cinder Timeline", &cinder );
        for( int i = 0; i < tween_count; ++i )
        {
          cinder_timeline->appendTo( &cinder_target, vec2( i * 5, i * 20 ), 2.0f ).delay( 1.0f );
        }

        ScopedTimer steps( "Cinder Steps" );
        for( float t = 0.0f; t <= 3.0f; t += dt ) {
          cinder_timeline->step( dt );
        }
      }

      cout << "Comparative timeline performance (Choreograph:Cinder): " << (choreograph / cinder) << endl;
    }
  }

  cout << endl;
}
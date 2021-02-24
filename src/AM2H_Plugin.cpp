#include "AM2H_Plugin.h"

class AM2H_Plugin{
  public:
    void activate() { active_ = true; }
    // inline const AM2H_Core& getCore(){if (core_) return *core_;}

    void call_loop(){ if ( active_ ) loop(); }
    virtual void loop()=0;

    /*void call_setup(AM2H_Core& core){
      if ( active_ && !setupDone_ ) { 
        core_ = &core;
        setup();
        setupDone_=true;
      }
    }*/
    virtual void setup()=0;

    /*void call_config(AM2H_Core& core, const char* key, const char* val){ 
      config( core, key, val );
    }
    virtual void config(AM2H_Core& core, const char* key, const char* val)=0;
    */

  protected:
    // AM2H_Core* core_ = nullptr;
    bool isActive() { return active_;}
    String srv_ {"none"};

  private:
    bool active_ = false;
    // bool interrupt_ = false;
    bool setupDone_ = false;
};

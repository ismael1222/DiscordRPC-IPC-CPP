# Discord RPC for C++

## Examples of use

### Bare minimum

```cpp
#include "DiscordRPC.hpp"
int main(){
    //get appID from https://discord.com/developers/applications
    std::string appID = "0000000000000000000";
    DiscordRPC RPC(appID);
    Activity act;
    RPC.setActivity(act);
    RPC.run();
    return 0;
}
```

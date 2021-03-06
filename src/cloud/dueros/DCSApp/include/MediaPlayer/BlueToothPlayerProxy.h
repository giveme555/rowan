/*
 * Copyright (c) 2017 Baidu, Inc. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef DUEROS_DCS_APP_MEDIAPLAYER_BLUETOOTHPLAYERPROXY_H_
#define DUEROS_DCS_APP_MEDIAPLAYER_BLUETOOTHPLAYERPROXY_H_

#include <DcsSdk/LocalMediaPlayerInterface.h>
#include "DCSApp/DeviceIoWrapper.h"

namespace duerOSDcsApp {
namespace mediaPlayer {

using namespace duerOSDcsSDK::sdkInterfaces;
using duerOSDcsSDK::sdkInterfaces::LocalMediaPlayerPlayActivity;

class BlueToothPlayerProxy : public LocalMediaPlayerInterface,
                             public application::DeviceIoWrapperObserverInterface {
public:
    static std::shared_ptr<BlueToothPlayerProxy> create();

    ~BlueToothPlayerProxy();

    void setObserver(std::shared_ptr<LocalMediaPlayerObserverInterface> playerObserver) override;

    LocalMediaPlayerStatus play() override;

    LocalMediaPlayerStatus stop() override;

    LocalMediaPlayerStatus pause() override;

    LocalMediaPlayerStatus resume() override;

    LocalMediaPlayerStatus playNext() override;

    LocalMediaPlayerStatus playPrevious() override;

    void btStartPlay() override;

    void btStopPlay() override;

    void btDisconnect() override;

    void btPowerOff() override;

    void setDcsSdk(std::shared_ptr<duerOSDcsSDK::sdkInterfaces::DcsSdk> dcsSdk);

private:
    BlueToothPlayerProxy();

private:
    std::shared_ptr<LocalMediaPlayerObserverInterface> m_playerObserver;

    LocalMediaPlayerPlayActivity m_localMediaPlayerPlayActivity;

    std::shared_ptr<duerOSDcsSDK::sdkInterfaces::DcsSdk> m_dcsSdk;
};

}  // mediaPlayer
}  // duerOSDcsApp

#endif  // DUEROS_DCS_APP_MEDIAPLAYER_BLUETOOTHPLAYERPROXY_H_

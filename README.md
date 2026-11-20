# BattleGround

Unreal Engine 5.7 기반 1대1 대전 프로젝트입니다.

## Windows에서 프로젝트 열기

최신 변경을 받은 뒤 저장소 루트의 **`OpenBattleGround.cmd`**를 실행합니다. Unreal Engine 5.7을 찾아 `BattleGroundEditor` 모듈을 빌드하고 에디터를 엽니다. 첫 실행에는 Unreal Engine과 Visual Studio 2022의 **C++를 사용한 게임 개발** 워크로드가 필요합니다. [UE 5.7 개발 환경](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine?application_version=5.7)

`.uproject` 실행 시 **Missing Modules**가 뜨면 `OpenBattleGround.cmd`로 빌드합니다. 빌드가 실패하면 창에 오류가 표시되고, UnrealBuildTool 로그는 `%LOCALAPPDATA%\UnrealBuildTool\Log.txt`에 있습니다. 엔진을 자동으로 찾지 못하면 설치 경로를 지정해 실행합니다.

```powershell
.\OpenBattleGround.cmd -EnginePath 'D:\Epic Games\UE_5.7'
```

## 방 열기와 참가

1. 두 플레이어가 같은 버전의 게임을 실행합니다.
2. 호스트가 **Create Room**을 누른 뒤 자신의 IP 주소와 대기 화면에 표시된 UDP 포트를 상대에게 전달합니다. 기본 포트는 `7777`입니다.
3. 상대가 **Host address**에 주소를 입력하고 **Join Room**을 누릅니다. 같은 PC에서 두 인스턴스를 실행할 때는 `127.0.0.1:7777`을 사용합니다.
4. 두 플레이어가 연결되면 대전이 시작됩니다. 상대 체력이 0이 되거나 상대가 경기 중 이탈하면 승리합니다. 한 방에서 한 판을 진행하며, `Esc` → **Return to Main Menu**로 돌아가 다음 방을 열 수 있습니다.

같은 네트워크에서는 호스트의 사설 IP를 사용합니다. 다른 네트워크에서는 상대가 접속할 수 있는 공인 IP 또는 호스트 이름이 필요합니다. 방화벽에서 호스트의 UDP 포트를 허용하고, 호스트가 NAT 공유기 뒤에 있으면 공유기의 해당 UDP 포트를 호스트 PC로 전달합니다.

## 개발 빌드와 로컬 검증

저장소 루트에서 `-Game` 옵션을 사용하면 빌드 후 에디터 대신 게임 창이 열립니다. 빌드만 하려면 `-BuildOnly`를 사용합니다.

```powershell
.\OpenBattleGround.cmd -Game
```

두 번째 PowerShell 창에서 같은 명령을 반복하거나 Windows로 패키징한 게임을 두 번 실행합니다. 다음 순서로 동작을 확인할 수 있습니다.

1. 첫 창에서 방을 열면 대기 인원이 `1/2`로 표시됩니다. 두 번째 창에서 `127.0.0.1`과 대기 화면의 포트로 접속합니다.
2. 두 플레이어가 서로 다른 위치에서 시작하고, 이동과 체력 변화가 두 창에 반영되는지 확인합니다. 기본 공격은 마우스 왼쪽 버튼입니다.
3. 경기 중 세 번째 창에서 같은 방에 접속하면 정원 초과 메시지가 표시됩니다.
4. 플레이어 한 명의 체력이 0이 되면 양쪽에 승패가 표시됩니다. 새 방에서 게스트가 경기 중 나가면 호스트가 기권승으로 처리되는지 확인합니다. 호스트가 방을 닫으면 게스트가 메뉴로 돌아옵니다.

## 별도 서버 프로세스

클라이언트 두 명과 별도로 서버를 실행할 때는 같은 Unreal 프로젝트를 서버 모드로 엽니다. 두 클라이언트는 서버의 주소와 포트로 접속합니다. 다음 경기를 시작하려면 서버 프로세스를 다시 실행합니다.

```powershell
$engine = 'C:\Program Files\Epic Games\UE_5.7\Engine'
$project = (Resolve-Path .\BattleGround.uproject).Path
& "$engine\Binaries\Win64\UnrealEditor.exe" $project '/Game/Map/BasicMap?game=BattleGroundDuel' -server -log -port=7777
```


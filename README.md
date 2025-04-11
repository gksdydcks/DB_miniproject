# DB_miniproject


### 한글 지원
- wide-character 사용
- wstring 이나 wchar_t 문자형을 다룰 때는 wcout, wcin 등 사용
  - std::wstring name = L"홍길동";
  - std::wcout << L"이름: " << name << std::endl;
    - 'L' 문자열 wcout은 wchar_t 타입 문자열만 출력 가능
   
| 종류      | 예시                                 | 용도                 |
|-----------|--------------------------------------|----------------------|
| `wchar_t` | `wchar_t ch = L'홍';`                | 유니코드 한 글자 저장 |
| `wstring` | `std::wstring name = L"홍길동";`     | 유니코드 문자열 저장 |
| `wcin`    | `std::wcin >> name;`                 | 유니코드 입력        |
| `wcout`   | `std::wcout << name;`                | 유니코드 출력        |

### 필수 코드

```cpp
#include <fcntl.h>
#include <io.h>
#include <locale>

// 콘솔을 유니코드로 전환
_setmode(_fileno(stdout), _O_U16TEXT);
_setmode(_fileno(stdin), _O_U16TEXT);
```

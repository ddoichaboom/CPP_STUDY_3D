# Assimp 분리 적용 가이드 — Editor 바이너리 변환 파이프라인

## 이 문서의 용도

이 문서는 `Framework_4Proj/Framework` (GameApp/Client/Engine/Editor 4-프로젝트 구조)에 Assimp 기반 모델 로딩을 적용할 때 **Claude에게 컨텍스트로 제공하기 위한 명세서**다.

핵심 원칙: **Assimp은 Editor에만 존재하고, Engine/Client/GameApp은 Assimp에 의존하지 않는다.**

---

## 배경

### Assimp 메모리 누수 문제

Assimp 라이브러리는 내부적으로 메모리 누수가 존재하며, 라이브러리 제작자도 인지하고 있는 알려진 이슈다. 따라서:
- 실무 프로젝트의 게임 런타임(Engine/Client)에 Assimp을 직접 링크하면 안 된다
- 별도 도구(Editor 또는 CLI 컨버터)에서 Assimp으로 모델을 로드한 뒤 필요한 데이터를 **커스텀 바이너리 포맷**으로 내보내고, 엔진은 이 바이너리만 읽는다

### 선택한 아키텍처: Editor 통합 방식

Editor 프로젝트에 이미 ImGui/ImGuizmo가 격리되어 있으므로, 같은 논리로 Assimp도 Editor에 격리한다.

```
[오프라인 — Editor.exe]
FBX/OBJ → Assimp 파싱 → 엔진 정점 포맷 변환 → .mesh/.model 바이너리 저장

[런타임 — GameApp.exe + Engine.dll + Client.dll]
.mesh/.model 바이너리 → fread → CreateBuffer → 렌더링
(Assimp 없음, 누수 없음)
```

---

## 현재 프로젝트 상태 (적용 전)

### 프로젝트 의존성

```
Engine (DLL)  ←  Client (DLL)  ←  GameApp (EXE)    [게임 런타임]
                                ←  Editor (EXE)     [개발 도구]
                                    ├─ ImGui (이미 격리)
                                    ├─ ImGuizmo (이미 격리)
                                    └─ Assimp (여기에 추가 예정)
```

### SDK 복사 흐름

```
Engine/Public/*.h  → EngineSDK/Inc/     (EngineSDK_Update.bat)
Engine/Bin/*.dll   → Client/Bin/, GameApp/Bin/, Editor/Bin/
Client/Public/*.h  → ClientSDK/Inc/     (ClientSDK_Update.bat)
Client/Bin/*.dll   → GameApp/Bin/, Editor/Bin/
```

### Engine 현재 정점 포맷 (Engine_Struct.h)

```cpp
typedef struct tagVertexPositionTexcoord
{
    XMFLOAT3    vPosition;
    XMFLOAT2    vTexcoord;
}VTXTEX;
```

현재 `VTXNORTEX`(Position+Normal+Texcoord)는 아직 없다. 수업 내용 적용 시 추가될 예정.

### Engine 현재 라이브러리 (Engine/ThirdPartyLib/)

- Effects11.lib / Effects11d.lib
- DirectXTK.lib / DirectXTKd.lib
- (Assimp 없음)

### Editor 현재 구현 (Editor/Private/)

- EditorApp.cpp — 기본 에디터 앱 (최소 구현)
- ImGui, ImGuizmo 소스 포함

### Engine_Defines.h 현재 상태

```cpp
#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <d3dcompiler.h>
#include <fx11/d3dx11effect.h>
#include <DirectXTK/DDSTextureLoader.h>
#include <DirectXTK/WICTextureLoader.h>
// Assimp 없음 — 의도적으로 Engine에 포함하지 않음
```

---

## 적용 계획: 단계별 구현

### Phase 1: 엔진 정점 포맷 확장

수업에서 배운 정점 포맷을 Engine_Struct.h에 추가한다. 이 포맷들은 바이너리 파일의 기반이 된다.

**Engine_Struct.h에 추가할 구조체:**

```cpp
// 위치 + 노멀 + 텍스처 좌표 (Terrain, Static Mesh용)
typedef struct tagVertexPositionNormalTexcoord
{
    XMFLOAT3    vPosition;
    XMFLOAT3    vNormal;
    XMFLOAT2    vTexcoord;

    static const unsigned int iNumElements = { 3 };
    static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
}VTXNORTEX;

// 향후 확장: 위치 + 노멀 + 탄젠트 + 텍스처 + 본 (스켈레탈 메시용)
// typedef struct tagVertexMesh { ... }VTXMESH;
```

### Phase 2: 바이너리 모델 포맷 정의 (Engine 공용)

Engine에 바이너리 포맷 구조체를 정의한다. Editor(쓰기)와 Engine(읽기) 양쪽에서 사용하므로 Engine_Struct.h에 배치.

```cpp
// === Engine_Struct.h에 추가 ===

// 바이너리 모델 파일 헤더
typedef struct tagModelFileHeader
{
    unsigned int    iVersion;           // 포맷 버전 (호환성 체크용)
    unsigned int    iNumMeshes;         // 메시 개수
    unsigned int    iNumMaterials;      // 재질 개수
    unsigned int    iVertexType;        // 정점 타입 식별자 (enum으로 관리)
}MODEL_FILE_HEADER;

// 메시 하나의 헤더
typedef struct tagMeshData
{
    unsigned int    iNumVertices;       // 정점 개수
    unsigned int    iNumIndices;        // 인덱스 개수
    unsigned int    iMaterialIndex;     // 참조하는 재질 인덱스
    unsigned int    iVertexStride;      // 정점 1개 크기 (바이트)
    // 이 헤더 뒤에 Vertices[iNumVertices], Indices[iNumIndices] 가 연속 배치
}MESH_DATA;

// 재질 데이터
typedef struct tagMaterialData
{
    wchar_t         szDiffuseTexture[MAX_PATH];     // 디퓨즈 텍스처 상대 경로
    wchar_t         szNormalTexture[MAX_PATH];      // 노멀맵 텍스처 상대 경로 (없으면 빈 문자열)
    XMFLOAT4        vAmbient;
    XMFLOAT4        vDiffuse;
    XMFLOAT4        vSpecular;
    float           fShininess;
}MATERIAL_DATA;
```

**바이너리 파일 레이아웃:**

```
[MODEL_FILE_HEADER]
[MESH_DATA #0][vertices bytes...][indices bytes...]
[MESH_DATA #1][vertices bytes...][indices bytes...]
...
[MATERIAL_DATA #0]
[MATERIAL_DATA #1]
...
```

### Phase 3: Editor에 Assimp 통합 + 변환기 구현

Editor 프로젝트에만 Assimp을 추가한다.

**Editor 프로젝트 설정 변경:**
- 추가 포함 디렉터리: `$(SolutionDir)Editor\assimp\` (Assimp 헤더 경로)
- 추가 라이브러리 디렉터리: `$(SolutionDir)Editor\ThirdPartyLib\` 또는 적절한 경로
- 추가 종속성: `assimp-vc143-mtd.lib` (Debug) / `assimp-vc143-mt.lib` (Release)
- Assimp DLL → `Editor/Bin/`에만 배치 (GameApp/Bin에는 넣지 않음!)

**Editor에 추가할 파일:**

```
Editor/
├── assimp/                  ← Assimp 헤더 (Editor 전용)
│   ├── scene.h
│   ├── Importer.hpp
│   ├── postprocess.h
│   └── ...
├── ThirdPartyLib/           ← Assimp 라이브러리
│   ├── assimp-vc143-mtd.lib
│   └── assimp-vc143-mt.lib
├── Public/
│   └── ModelConverter.h     ← NEW: 모델 변환기 클래스
├── Private/
│   └── ModelConverter.cpp   ← NEW: 변환 로직 구현
└── Bin/
    └── assimp-vc143-mtd.dll ← Assimp DLL (Editor 실행 시에만 필요)
```

**ModelConverter 클래스 설계:**

```cpp
// === Editor/Public/ModelConverter.h ===
#pragma once

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

// Engine 헤더 참조 (EngineSDK 경유)
#include "Engine_Struct.h"
#include "Engine_Typedef.h"

namespace Editor
{
    class CModelConverter
    {
    public:
        // FBX/OBJ 파일을 읽어서 커스텀 바이너리(.model)로 저장
        static HRESULT Convert(const char* pSourcePath, const wchar_t* pOutputPath);

    private:
        // aiScene에서 메시 데이터 추출
        static HRESULT Extract_Meshes(const aiScene* pScene,
            std::vector<Engine::MESH_DATA>& outMeshHeaders,
            std::vector<std::vector<Engine::VTXNORTEX>>& outVertices,
            std::vector<std::vector<unsigned int>>& outIndices);

        // aiScene에서 재질 데이터 추출
        static HRESULT Extract_Materials(const aiScene* pScene,
            const char* pModelDirectory,
            std::vector<Engine::MATERIAL_DATA>& outMaterials);

        // 바이너리 파일로 직렬화
        static HRESULT Write_Binary(const wchar_t* pOutputPath,
            const Engine::MODEL_FILE_HEADER& header,
            const std::vector<Engine::MESH_DATA>& meshHeaders,
            const std::vector<std::vector<Engine::VTXNORTEX>>& vertices,
            const std::vector<std::vector<unsigned int>>& indices,
            const std::vector<Engine::MATERIAL_DATA>& materials);
    };
}
```

**Convert() 핵심 흐름:**

```cpp
HRESULT CModelConverter::Convert(const char* pSourcePath, const wchar_t* pOutputPath)
{
    // (1) Assimp으로 FBX/OBJ 로드
    Assimp::Importer importer;
    unsigned int iFlags = aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast;
    const aiScene* pScene = importer.ReadFile(pSourcePath, iFlags);
    if (!pScene) return E_FAIL;

    // (2) 메시 데이터 추출 → 엔진 정점 포맷(VTXNORTEX)으로 변환
    std::vector<MESH_DATA> meshHeaders;
    std::vector<std::vector<VTXNORTEX>> allVertices;
    std::vector<std::vector<unsigned int>> allIndices;
    Extract_Meshes(pScene, meshHeaders, allVertices, allIndices);

    // (3) 재질 데이터 추출
    std::vector<MATERIAL_DATA> materials;
    Extract_Materials(pScene, /* modelDir */, materials);

    // (4) 헤더 구성
    MODEL_FILE_HEADER header{};
    header.iVersion = 1;
    header.iNumMeshes = (unsigned int)meshHeaders.size();
    header.iNumMaterials = (unsigned int)materials.size();
    header.iVertexType = 0; // VTXNORTEX 식별자

    // (5) 바이너리 쓰기
    Write_Binary(pOutputPath, header, meshHeaders, allVertices, allIndices, materials);

    // (6) importer 소멸 → aiScene 자동 해제 → Assimp 누수는 Editor 프로세스에 한정
    return S_OK;
}
```

**Extract_Meshes() 핵심 — aiMesh → VTXNORTEX 변환:**

```cpp
// aiMesh 하나를 엔진 정점 포맷으로 변환하는 핵심 로직
for (unsigned int m = 0; m < pScene->mNumMeshes; ++m)
{
    aiMesh* pMesh = pScene->mMeshes[m];

    MESH_DATA meshData{};
    meshData.iNumVertices = pMesh->mNumVertices;
    meshData.iVertexStride = sizeof(VTXNORTEX);
    meshData.iMaterialIndex = pMesh->mMaterialIndex;

    std::vector<VTXNORTEX> vertices(pMesh->mNumVertices);
    for (unsigned int v = 0; v < pMesh->mNumVertices; ++v)
    {
        // Position
        vertices[v].vPosition.x = pMesh->mVertices[v].x;
        vertices[v].vPosition.y = pMesh->mVertices[v].y;
        vertices[v].vPosition.z = pMesh->mVertices[v].z;

        // Normal
        if (pMesh->HasNormals())
        {
            vertices[v].vNormal.x = pMesh->mNormals[v].x;
            vertices[v].vNormal.y = pMesh->mNormals[v].y;
            vertices[v].vNormal.z = pMesh->mNormals[v].z;
        }

        // Texcoord (UV 채널 0)
        if (pMesh->HasTextureCoords(0))
        {
            vertices[v].vTexcoord.x = pMesh->mTextureCoords[0][v].x;
            vertices[v].vTexcoord.y = pMesh->mTextureCoords[0][v].y;
        }
    }

    // Indices — aiMesh의 Face 배열에서 추출
    std::vector<unsigned int> indices;
    for (unsigned int f = 0; f < pMesh->mNumFaces; ++f)
    {
        aiFace& face = pMesh->mFaces[f];
        for (unsigned int idx = 0; idx < face.mNumIndices; ++idx)
            indices.push_back(face.mIndices[idx]);
    }
    meshData.iNumIndices = (unsigned int)indices.size();

    // 결과 저장
    meshHeaders.push_back(meshData);
    allVertices.push_back(std::move(vertices));
    allIndices.push_back(std::move(indices));
}
```

**Extract_Materials() 핵심 — aiMaterial → MATERIAL_DATA:**

```cpp
for (unsigned int m = 0; m < pScene->mNumMaterials; ++m)
{
    aiMaterial* pMtrl = pScene->mMaterials[m];
    MATERIAL_DATA mtrlData{};

    // 디퓨즈 텍스처 경로
    aiString texPath;
    if (pMtrl->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
    {
        // char → wchar_t 변환 후 상대 경로로 저장
        MultiByteToWideChar(CP_UTF8, 0, texPath.C_Str(), -1,
            mtrlData.szDiffuseTexture, MAX_PATH);
    }

    // 노멀맵 텍스처 경로
    if (pMtrl->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS)
    {
        MultiByteToWideChar(CP_UTF8, 0, texPath.C_Str(), -1,
            mtrlData.szNormalTexture, MAX_PATH);
    }

    // 색상 속성
    aiColor4D color;
    if (pMtrl->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS)
        mtrlData.vAmbient = XMFLOAT4(color.r, color.g, color.b, color.a);
    if (pMtrl->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
        mtrlData.vDiffuse = XMFLOAT4(color.r, color.g, color.b, color.a);
    if (pMtrl->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
        mtrlData.vSpecular = XMFLOAT4(color.r, color.g, color.b, color.a);

    float shininess = 0.f;
    pMtrl->Get(AI_MATKEY_SHININESS, shininess);
    mtrlData.fShininess = shininess;

    materials.push_back(mtrlData);
}
```

**Write_Binary() — 바이너리 직렬화:**

```cpp
HRESULT CModelConverter::Write_Binary(const wchar_t* pOutputPath, ...)
{
    FILE* pFile = nullptr;
    _wfopen_s(&pFile, pOutputPath, L"wb");
    if (!pFile) return E_FAIL;

    // (1) 파일 헤더
    fwrite(&header, sizeof(MODEL_FILE_HEADER), 1, pFile);

    // (2) 메시 데이터 (헤더 + 정점 + 인덱스) 반복
    for (unsigned int m = 0; m < header.iNumMeshes; ++m)
    {
        fwrite(&meshHeaders[m], sizeof(MESH_DATA), 1, pFile);
        fwrite(vertices[m].data(), sizeof(VTXNORTEX), meshHeaders[m].iNumVertices, pFile);
        fwrite(indices[m].data(), sizeof(unsigned int), meshHeaders[m].iNumIndices, pFile);
    }

    // (3) 재질 데이터
    for (unsigned int m = 0; m < header.iNumMaterials; ++m)
        fwrite(&materials[m], sizeof(MATERIAL_DATA), 1, pFile);

    fclose(pFile);
    return S_OK;
}
```

### Phase 4: Engine의 CModel — 바이너리 읽기 전용

Engine의 CModel은 Assimp 없이 바이너리만 읽는다.

```cpp
// === Engine/Public/Model.h ===
#pragma once
#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL CModel final : public CComponent
{
private:
    CModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CModel(const CModel& Prototype);
    virtual ~CModel() = default;

public:
    virtual HRESULT Initialize_Prototype(const _tchar* pModelBinaryPath);
    virtual HRESULT Initialize(void* pArg);

public:
    _uint Get_NumMeshes() const { return m_iNumMeshes; }
    HRESULT Bind_Resources(_uint iMeshIndex);
    HRESULT Render(_uint iMeshIndex);

private:
    HRESULT Load_Binary(const _tchar* pFilePath);

private:
    _uint                                   m_iNumMeshes = {};
    vector<ID3D11Buffer*>                   m_VBs;          // 메시별 VB
    vector<ID3D11Buffer*>                   m_IBs;          // 메시별 IB
    vector<_uint>                           m_NumIndices;   // 메시별 인덱스 개수
    vector<_uint>                           m_VertexStrides;
    // 재질/텍스처 관련 멤버는 프로젝트 진행에 따라 확장

public:
    static CModel* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
                          const _tchar* pModelBinaryPath);
    virtual CComponent* Clone(void* pArg) override;
    virtual void Free() override;
};

NS_END
```

**Load_Binary() 핵심 — fread → CreateBuffer:**

```cpp
HRESULT CModel::Load_Binary(const _tchar* pFilePath)
{
    FILE* pFile = nullptr;
    _wfopen_s(&pFile, pFilePath, L"rb");
    if (!pFile) return E_FAIL;

    // (1) 파일 헤더 읽기
    MODEL_FILE_HEADER header{};
    fread(&header, sizeof(MODEL_FILE_HEADER), 1, pFile);

    if (header.iVersion != 1) { fclose(pFile); return E_FAIL; }

    m_iNumMeshes = header.iNumMeshes;

    // (2) 메시별 VB/IB 생성
    for (_uint m = 0; m < m_iNumMeshes; ++m)
    {
        MESH_DATA meshData{};
        fread(&meshData, sizeof(MESH_DATA), 1, pFile);

        // 정점 데이터 읽기
        void* pVertices = malloc(meshData.iVertexStride * meshData.iNumVertices);
        fread(pVertices, meshData.iVertexStride, meshData.iNumVertices, pFile);

        // 인덱스 데이터 읽기
        _uint* pIndices = new _uint[meshData.iNumIndices];
        fread(pIndices, sizeof(_uint), meshData.iNumIndices, pFile);

        // Vertex Buffer 생성
        D3D11_BUFFER_DESC vbDesc{};
        vbDesc.ByteWidth = meshData.iVertexStride * meshData.iNumVertices;
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vbData{};
        vbData.pSysMem = pVertices;

        ID3D11Buffer* pVB = nullptr;
        m_pDevice->CreateBuffer(&vbDesc, &vbData, &pVB);
        m_VBs.push_back(pVB);

        // Index Buffer 생성
        D3D11_BUFFER_DESC ibDesc{};
        ibDesc.ByteWidth = sizeof(_uint) * meshData.iNumIndices;
        ibDesc.Usage = D3D11_USAGE_DEFAULT;
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA ibData{};
        ibData.pSysMem = pIndices;

        ID3D11Buffer* pIB = nullptr;
        m_pDevice->CreateBuffer(&ibDesc, &ibData, &pIB);
        m_IBs.push_back(pIB);

        m_NumIndices.push_back(meshData.iNumIndices);
        m_VertexStrides.push_back(meshData.iVertexStride);

        free(pVertices);
        delete[] pIndices;
    }

    // (3) 재질 데이터 읽기 (텍스처 로딩 등은 프로젝트 진행에 따라 확장)
    // ...

    fclose(pFile);
    return S_OK;
}
```

### Phase 5: Editor UI — ImGui 변환 인터페이스

EditorApp에 모델 변환 UI를 추가한다.

```cpp
// === Editor에서 ImGui를 통한 변환 UI 예시 ===
void CEditorApp::Render_ModelConverter()
{
    ImGui::Begin("Model Converter");

    static char szSourcePath[MAX_PATH] = {};
    ImGui::InputText("Source (FBX/OBJ)", szSourcePath, MAX_PATH);

    static wchar_t szOutputPath[MAX_PATH] = {};
    static char szOutputPathA[MAX_PATH] = {};
    ImGui::InputText("Output (.model)", szOutputPathA, MAX_PATH);

    if (ImGui::Button("Convert"))
    {
        MultiByteToWideChar(CP_UTF8, 0, szOutputPathA, -1, szOutputPath, MAX_PATH);
        HRESULT hr = CModelConverter::Convert(szSourcePath, szOutputPath);
        if (SUCCEEDED(hr))
            ImGui::OpenPopup("Success");
        else
            ImGui::OpenPopup("Failed");
    }

    // 결과 팝업
    if (ImGui::BeginPopupModal("Success", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Model converted successfully!");
        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::End();
}
```

---

## 빌드 설정 요약

### Engine 프로젝트 — 변경 없음 (Assimp 비포함)

| 항목 | 값 |
|------|-----|
| 추가 포함 디렉터리 | 변경 없음 |
| 추가 라이브러리 | 변경 없음 |
| 추가 종속성 | 변경 없음 |
| 새 파일 | Model.h, Model.cpp |

Engine_Defines.h에 Assimp include를 **추가하지 않는다**. 이것이 핵심이다.

### Editor 프로젝트 — Assimp 추가

| 항목 | 값 |
|------|-----|
| 추가 포함 디렉터리 | `$(SolutionDir)Editor\assimp\` 추가 |
| 추가 라이브러리 디렉터리 | `$(SolutionDir)Editor\ThirdPartyLib\` 추가 |
| 추가 종속성 (Debug) | `assimp-vc143-mtd.lib` |
| 추가 종속성 (Release) | `assimp-vc143-mt.lib` |
| DLL 배치 | `Editor/Bin/assimp-vc143-mtd.dll` |
| 새 파일 | ModelConverter.h, ModelConverter.cpp |

### GameApp / Client 프로젝트 — 변경 없음

Assimp 관련 설정 일절 없음. .model 바이너리만 Resources에 배치.

---

## 파일 배치 최종 구조

```
Framework/
├── Engine/
│   ├── Public/
│   │   ├── Engine_Struct.h      ← MODEL_FILE_HEADER, MESH_DATA, MATERIAL_DATA 추가
│   │   ├── Model.h              ← NEW: 바이너리 읽기 전용 CModel
│   │   └── (기존 파일들)
│   ├── Private/
│   │   ├── Model.cpp            ← NEW: Load_Binary() 구현
│   │   └── (기존 파일들)
│   └── ThirdPartyLib/           ← Assimp 없음!
│
├── Editor/
│   ├── assimp/                  ← NEW: Assimp 헤더 (Editor 전용)
│   ├── ThirdPartyLib/           ← NEW: Assimp 라이브러리
│   ├── Public/
│   │   └── ModelConverter.h     ← NEW
│   ├── Private/
│   │   ├── EditorApp.cpp        ← 수정: 변환 UI 추가
│   │   └── ModelConverter.cpp   ← NEW
│   ├── Bin/
│   │   └── assimp-vc143-mtd.dll ← NEW: Editor 실행 시에만 필요
│   └── ImGui/                   ← 기존
│
├── Resources/
│   └── Models/
│       ├── Fiona.fbx            ← 원본 (Editor가 읽음)
│       ├── Fiona.model          ← 변환 결과 (Engine이 읽음)
│       └── ...
│
├── GameApp/Bin/                 ← assimp DLL 없음!
└── Client/Bin/                  ← assimp DLL 없음!
```

---

## 개발 워크플로우

```
[모델 추가/수정 시]
1. 아티스트가 FBX 파일을 Resources/Models/에 배치
2. Editor.exe 실행
3. Model Converter UI에서 Source에 FBX 경로 입력
4. Output에 .model 경로 입력
5. "Convert" 클릭 → .model 바이너리 생성
6. Editor 종료 (Assimp 누수는 프로세스와 함께 소멸)

[게임 실행 시]
1. GameApp.exe 실행
2. Engine의 CModel::Load_Binary()로 .model 파일 로드
3. fread → CreateBuffer → 렌더링
4. Assimp 없음 → 누수 없음
```

---

## 향후 확장 고려사항

### 스켈레탈 애니메이션

수업이 진행되면서 본(Bone), 키프레임, 애니메이션 데이터가 추가될 것이다. 바이너리 포맷을 확장하면 된다:

```cpp
// MODEL_FILE_HEADER에 추가
unsigned int    iNumBones;
unsigned int    iNumAnimations;

// 새 구조체
typedef struct tagBoneData { ... }BONE_DATA;
typedef struct tagAnimationData { ... }ANIMATION_DATA;
typedef struct tagKeyFrameData { ... }KEYFRAME_DATA;
```

포맷 버전(`iVersion`)을 올려서 하위 호환성을 유지한다.

### 정점 포맷 분기

모델에 따라 정점 포맷이 다를 수 있다 (Normal만 있는 것 vs Tangent도 있는 것). `MODEL_FILE_HEADER.iVertexType`으로 구분:

```cpp
enum class VERTEX_TYPE { VTXNORTEX, VTXMESH, VTXANIMMESH, END };
```

Engine의 Load_Binary()에서 타입에 따라 분기하여 적절한 InputLayout과 매칭.

---

## 주의사항

1. **바이너리 파일은 플랫폼 종속적이다**: x86/x64, 엔디안이 다르면 호환되지 않는다. 같은 빌드 설정에서 Editor로 변환하고 GameApp으로 읽어야 한다.
2. **구조체 패딩에 주의**: `#pragma pack(push, 1)` 사용을 고려하거나, 필드 순서를 정렬에 맞게 배치한다.
3. **텍스처 경로는 상대 경로로 저장**: FBX 내부의 절대 경로를 Resources 기준 상대 경로로 변환해야 한다.
4. **Assimp Importer는 지역 변수로**: Convert() 함수 내에서 생성하고 함수 종료 시 소멸하도록 한다. 멤버 변수로 오래 들고 있으면 누수가 누적된다.
5. **Editor에서 EngineSDK 헤더를 사용**: 바이너리 포맷 구조체(MODEL_FILE_HEADER 등)는 Engine_Struct.h에 정의하고, Editor는 EngineSDK/Inc/ 경유로 참조한다. 양쪽이 같은 구조체를 사용해야 읽기/쓰기가 일치한다.

# Публикация Arena DDS Optimizer на GitHub

## 1. Создание репозитория

Создайте пустой репозиторий, например `ArenaDDSOptimizer`, затем из корня проекта:

```bash
git init
git add .
git commit -m "Initial Arena DDS Optimizer"
git branch -M main
git remote add origin https://github.com/ВАШ_АККАУНТ/ArenaDDSOptimizer.git
git push -u origin main
```

После push workflow **Build Windows** автоматически соберёт `ArenaDDSOptimizer-Windows-x64.zip`.

В ZIP уже будут `ArenaDDSOptimizer.exe`, Qt DLL и `texconv.exe`.

## 2. DirectXTex / Texconv

Workflow не хранит готовый `texconv.exe` в репозитории. Он клонирует официальный Microsoft DirectXTex с зафиксированного тега `may2026`, собирает target `texconv` и включает результат в portable ZIP.

Это делает сборку воспроизводимой. В релиз также копируется `DirectXTex-LICENSE.txt` (MIT).

## 3. Создание релиза

```bash
git tag v0.1.3
git push origin v0.1.3
```

Workflow **Release Windows** соберёт portable ZIP и создаст GitHub Release автоматически.

## 4. Ветки

Рекомендуется:

- `main` — стабильная версия;
- feature-ветки → Pull Request → Windows Build + CodeQL;
- релизы — тегами `vX.Y.Z`.

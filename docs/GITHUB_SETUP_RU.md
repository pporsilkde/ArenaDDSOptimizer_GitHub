# Публикация Arena DDS Optimizer на GitHub

## 1. Создание репозитория

Создайте пустой репозиторий, например `ArenaDDSOptimizer`, без автоматического README/License, затем из корня проекта:

```bash
git init
git add .
git commit -m "Initial Arena DDS Optimizer"
git branch -M main
git remote add origin https://github.com/ВАШ_АККАУНТ/ArenaDDSOptimizer.git
git push -u origin main
```

После push workflow `Build` автоматически соберёт:

- `ArenaDDSOptimizer-Windows-x64.zip` — portable Windows build с Qt DLL;
- `ArenaDDSOptimizer-Linux-x86_64.AppImage` — переносимый Linux build.

Артефакты находятся в **Actions → Build → Artifacts**.

## 2. Создание релиза

```bash
git tag v0.1.0
git push origin v0.1.0
```

Workflow `Release` соберёт обе платформы, создаст GitHub Release и приложит готовые бинарники.

## 3. texconv

`texconv` не включается в репозиторий и релиз. Пользователь выбирает установленный `texconv.exe` в интерфейсе. Это сохраняет чистое разделение лицензий и позволяет обновлять DirectXTex независимо.

## 4. Ветки

Рекомендуется:

- `main` — рабочая стабильная версия;
- feature-ветки → Pull Request → CI Build + CodeQL;
- release — только через тег `vX.Y.Z`.

param(
    [Parameter(Mandatory = $true)]
    [string]$GameRoot,
    [string]$DotNet = 'dotnet'
)
$ErrorActionPreference = 'Stop'

$game = Get-Item -LiteralPath $GameRoot
if (!$game.PSIsContainer -or !(Test-Path -LiteralPath (Join-Path $game.FullName 'Suzerain.exe') -PathType Leaf)) {
    throw 'GameRoot must be the Suzerain installation directory containing Suzerain.exe.'
}
$dotnetExe = (Get-Command $DotNet -CommandType Application -ErrorAction Stop).Source
$project = Join-Path $PSScriptRoot 'SuzerainSaveStudio.Live.csproj'
$repo = Split-Path -Parent $PSScriptRoot
$obj = (Join-Path $repo 'build/obj/live').Replace('\', '/') + '/'
$output = Join-Path $repo 'build/live/SuzerainSaveStudio.Live.dll'

& $dotnetExe build $project --configuration Release --nologo `
    "-p:GameRoot=$($game.FullName)" "-p:BaseIntermediateOutputPath=$obj" `
    '-p:NuGetAudit=false'
if ($LASTEXITCODE -ne 0 -or !(Test-Path -LiteralPath $output -PathType Leaf)) {
    throw 'Live plugin build failed. Use a .NET SDK with the .NET 6 targeting pack and the local BepInEx 6 IL2CPP interop assemblies.'
}
Write-Output "Built $output"

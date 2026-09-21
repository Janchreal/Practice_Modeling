param(
    [string]$ProjectPath = (Join-Path $PSScriptRoot "..\Practice_Modeling.vcxproj"),
    [string]$FiltersPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Normalize-ProjectPath {
    param([string]$Path)
    return ($Path -replace '/', '\').Trim()
}

function Normalize-IncludePath {
    param(
        [string]$Path,
        [string]$ProjectDirectory
    )

    $normalized = Normalize-ProjectPath $Path
    if (-not [System.IO.Path]::IsPathRooted($normalized)) {
        return $normalized
    }

    $fullPath = [System.IO.Path]::GetFullPath($normalized)
    $projectRoot = ([System.IO.Path]::GetFullPath($ProjectDirectory)).TrimEnd('\') + '\'
    if ($fullPath.StartsWith($projectRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return Normalize-ProjectPath $fullPath.Substring($projectRoot.Length)
    }

    return $normalized
}

function Get-FilterPath {
    param([string]$IncludePath)

    $normalized = Normalize-ProjectPath $IncludePath
    $parent = Split-Path -Path $normalized -Parent
    if ([string]::IsNullOrWhiteSpace($parent)) {
        return $null
    }

    return (Normalize-ProjectPath $parent).TrimEnd('\')
}

function Get-StableGuid {
    param([string]$Text)

    $md5 = [System.Security.Cryptography.MD5]::Create()
    try {
        $bytes = $md5.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($Text))
        return ([guid]::new($bytes)).ToString("B").ToUpperInvariant()
    }
    finally {
        $md5.Dispose()
    }
}

function Add-FilterAndParents {
    param(
        [System.Collections.Generic.HashSet[string]]$Filters,
        [string]$FilterPath
    )

    if ([string]::IsNullOrWhiteSpace($FilterPath)) {
        return
    }

    $parts = (Normalize-ProjectPath $FilterPath) -split '\\' | Where-Object { $_ }
    $current = ""
    foreach ($part in $parts) {
        if ($current.Length -eq 0) {
            $current = $part
        }
        else {
            $current = "$current\$part"
        }
        [void]$Filters.Add($current)
    }
}

$projectFullPath = [System.IO.Path]::GetFullPath($ProjectPath)
if (-not (Test-Path -LiteralPath $projectFullPath)) {
    throw "Project file not found: $projectFullPath"
}
$projectDirectory = [System.IO.Path]::GetDirectoryName($projectFullPath)

if ([string]::IsNullOrWhiteSpace($FiltersPath)) {
    $FiltersPath = "$projectFullPath.filters"
}
$filtersFullPath = [System.IO.Path]::GetFullPath($FiltersPath)

[xml]$project = Get-Content -LiteralPath $projectFullPath -Raw
$namespaceUri = $project.DocumentElement.NamespaceURI
$ns = New-Object System.Xml.XmlNamespaceManager($project.NameTable)
$ns.AddNamespace("msb", $namespaceUri)

$itemTypes = @(
    "ClCompile",
    "ClInclude",
    "CustomBuild",
    "None",
    "ResourceCompile",
    "Image",
    "Text",
    "Xml"
)

$items = New-Object System.Collections.Generic.List[object]
$filters = New-Object 'System.Collections.Generic.HashSet[string]'

foreach ($itemType in $itemTypes) {
    $nodes = $project.SelectNodes("//msb:$itemType[@Include]", $ns)
    foreach ($node in $nodes) {
        $includePath = Normalize-IncludePath -Path $node.Include -ProjectDirectory $projectDirectory
        $filterPath = Get-FilterPath $includePath
        Add-FilterAndParents -Filters $filters -FilterPath $filterPath

        $items.Add([pscustomobject]@{
            ItemType = $itemType
            Include = $includePath
            Filter = $filterPath
        })
    }
}

$settings = New-Object System.Xml.XmlWriterSettings
$settings.Indent = $true
$settings.IndentChars = "  "
$settings.NewLineChars = "`r`n"
$settings.Encoding = New-Object System.Text.UTF8Encoding($false)

$writer = [System.Xml.XmlWriter]::Create($filtersFullPath, $settings)
try {
    $writer.WriteStartDocument()
    $writer.WriteStartElement("Project", $namespaceUri)

    $writer.WriteStartElement("ItemGroup")
    foreach ($filter in ($filters | Sort-Object)) {
        $writer.WriteStartElement("Filter")
        $writer.WriteAttributeString("Include", $filter)
        $writer.WriteElementString("UniqueIdentifier", (Get-StableGuid "vcxproj-filter:$filter"))
        $writer.WriteEndElement()
    }
    $writer.WriteEndElement()

    foreach ($group in ($items | Group-Object ItemType)) {
        $writer.WriteStartElement("ItemGroup")
        foreach ($item in $group.Group) {
            $writer.WriteStartElement($item.ItemType)
            $writer.WriteAttributeString("Include", $item.Include)
            if (-not [string]::IsNullOrWhiteSpace($item.Filter)) {
                $writer.WriteElementString("Filter", $item.Filter)
            }
            $writer.WriteEndElement()
        }
        $writer.WriteEndElement()
    }

    $writer.WriteEndElement()
    $writer.WriteEndDocument()
}
finally {
    $writer.Dispose()
}

Write-Host "Synced $($items.Count) items into $($filters.Count) physical-directory filters: $filtersFullPath"

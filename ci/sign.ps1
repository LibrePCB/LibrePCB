$FILE=$args[0]

if ($env:AST_TENANT) {
    AzureSignTool.exe sign `
        -kvu "$env:AST_VAULT" `
        -kvc "$env:AST_CERT" `
        -kvi "$env:AST_IDENT" `
        -kvs "$env:AST_SECRET" `
        --azure-key-vault-tenant-id "$env:AST_TENANT" `
        -tr "$env:AST_TIMESTAMP" `
        -td $env:AST_TD `
        $FILE
} else {
    Write-Host "Skipped code signing of $FILE"
}

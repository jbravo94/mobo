if (!((Get-Date).DayOfYear % 2 -eq 0)) {
    aws lambda invoke --function-name backup_buddy_api --payload '{\"requestType\": \"reportBackupStatusForApplication\", \"applicationName\": \"jira\", \"backupStatus\": \"SKIPPED\"}' --cli-binary-format raw-in-base64-out C:\Temp\aws-response.json
    echo "SKIPPED."
    return
}

echo "PERFORM BACKUP."

aws lambda invoke --function-name backup_buddy_api --payload '{\"requestType\": \"reportBackupStatusForApplication\", \"applicationName\": \"jira\", \"backupStatus\": \"SUCCESS\"}' --cli-binary-format raw-in-base64-out C:\Temp\aws-response.json

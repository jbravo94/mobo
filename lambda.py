import json
import logging
from datetime import datetime
import boto3
from boto3.dynamodb.conditions import Key, Attr

logger = logging.getLogger()
logger.setLevel("INFO")

client = boto3.client('dynamodb')
dynamodb = boto3.resource("dynamodb")
table = dynamodb.Table('backup_buddy_db')

def lambda_handler(event, context):
    
    logger.info(event)

    """
    response = {
                "description": "Backup Monitoring",
                "lastUpdated": current_timestamp.isoformat(),
                "failedBackups": 1
            }

    return {
                'statusCode': 200,
                'body': json.dumps(response)
            }
    """

 

    current_timestamp = datetime.now()

    current_timestamp_seconds = int(current_timestamp.timestamp())

    current_date_timestamp_seconds = int(datetime(current_timestamp.year, current_timestamp.month, current_timestamp.day, 0, 0, 0).timestamp())

    if event.get("body") is not None:
        body = json.loads(event["body"])
    else:
        body = event

    match body["requestType"]:
        case "getRecentBackupStatus":

            thresholds = table.get_item(Key={'id': 'configuration', 'timestamp': 0})

            applications_to_monitor = thresholds['Item']['applications_to_monitor'].split(",")

            applications_to_monitor_threshold = len(applications_to_monitor)


            response = table.query(
                Select="ALL_ATTRIBUTES",
                KeyConditionExpression=Key('id').eq('backup_event') & Key('timestamp').gte(current_date_timestamp_seconds)
            )

            recent_backup_status = {}

            for item in response['Items']:

                application_name = item['application_name']

                if application_name not in applications_to_monitor:
                    continue

                if recent_backup_status.get(item['application_name']) is None:
                    recent_backup_status[application_name] = item
                    continue
                
                if recent_backup_status[application_name]['timestamp'] < item['timestamp']:
                    recent_backup_status[application_name] = item
                
            successful_backups = 0
    
            logger.info(response)

            for item in recent_backup_status.values():
                if item['backup_status'] in ["SUCCESS", "SKIPPED"]:
                    successful_backups += 1

            response = {
                "description": "Backup Monitoring",
                "lastUpdated": current_timestamp.isoformat(),
                "failedBackups": applications_to_monitor_threshold - successful_backups
            }

            return {
                'statusCode': 200,
                'body': json.dumps(response)
            }

        case "reportBackupStatusForApplication":
            application_name = body["applicationName"]
            backup_status = body["backupStatus"]

            table.put_item(
                Item={
                    'id': 'backup_event',
                    'application_name': application_name,
                    'backup_status': backup_status,
                    'timestamp': current_timestamp_seconds
                }
            )

            response = {
                "message": "Saved successfully."
            }

            return {
                'statusCode': 200,
                'body': json.dumps(response)
            }

        case _:

            response = {
                "message": "Request type not supported."
            }

            return {
                'statusCode': 200,
                'body': json.dumps(response)
            }


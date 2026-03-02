from confluent_kafka import Consumer,KafkaError,KafkaException
import json
import signal
import sys

running=True

def signal_handler(sig,frame):
    global running
    print("shutdown signal received,closing consumer gracefully...")
    running = False

def process_camera_stream(source_id,payload):
    if source_id=="testid1":
        print(f"Test Camera Processing frame {payload.get('task_details',{}).get('frame_id')}...")
    elif source_id=="1bfa0d514984801bb4ddbd760bfe9c5":
        print(f"Test Camera Processing frame {payload.get('task_details',{}).get('frame_id')}...")
    else:
        print(f"UNKNOWN CAMERA source : {source_id} | Payload keys : {list(payload.keys())}")

def main():
    global running
    signal.signal(signal.SIGINT,signal_handler)
    signal.signal(signal.SIGTERM,signal_handler)
    conf={
        "bootstrap.servers":"localhost:9093",
        "group.id":"deepstream-analytics-processor",
        "auto.offset.reset":"latest",
        "enable.auto.commit":True
    }
    consumer=Consumer(conf)
    topic="deepstream-analytics"
    consumer.subscribe([topic])
    print(f"[INFO] Subscribed to topic: {topic}. Waiting for frames...")
    try:
        while running:
            # 4. Polling with a timeout
            msg = consumer.poll(timeout=1.0)

            if msg is None:
                continue

            if msg.error():
                if msg.error().code() == KafkaError._PARTITION_EOF:
                    # End of partition event (not an actual error)
                    continue
                else:
                    raise KafkaException(msg.error())

            # 5. Robust Extraction
            try:
                # Extract the Key (Your source_id)
                raw_key = msg.key()
                source_id = raw_key.decode('utf-8') if raw_key else "UNKNOWN"
                print(raw_key.decode('utf-8'))
                # Extract and parse the Value (Your JSON payload)
                raw_value = msg.value()
                payload = json.loads(raw_value.decode('utf-8'))

                # 6. Route the data
                process_camera_stream(source_id, payload)

            except json.JSONDecodeError:
                print(f"[ERROR] Received malformed JSON on partition {msg.partition()}")
            except Exception as e:
                print(f"[ERROR] Processing failure: {str(e)}")

    finally:
        # 7. Graceful Teardown
        # This commits final offsets and leaves the consumer group cleanly
        consumer.close()
        print("[INFO] Consumer closed. Exiting.")

if __name__ == '__main__':
    main()

import mqtt from "mqtt";

export const client = mqtt.connect("ws://5.61.89.44:9001")

let handlers = new Map();

export function messageHandler(topic, handler) {
    if (!handlers.has(topic)) {
        client.subscribe(topic, (err) => {
            if (err) {
                console.log(err)
            }
        });
        handlers.set(topic, new Array());
    }

    handlers.get(topic).push(handler)
}

client.on("connect", () => {
    client.on("message", (topic, message) => {
        try {
            handlers.get(topic).forEach(handler => handler(message));
        } catch (err) {
            console.log(err)
        }
    })
});
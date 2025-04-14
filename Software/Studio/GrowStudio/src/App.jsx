import { useState, useEffect } from 'react'
import reactLogo from './assets/react.svg'
import viteLogo from '/vite.svg'
import './App.css'
import { client, messageHandler } from "./MqttApi"


function Slider({name, unit, onChange, min, max, step="1"}) {
  const [value, setValue] = useState(0)
  return (
    <div className="col">
      <label>{name}: {value} {unit}</label>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) =>  {
          setValue(Number(e.target.value));
          onChange(Number(e.target.value));
        }}
      />
    </div>
  );
}

function Slider2({name, unit, value, onChange, min, max, step="1"}) {
  return (
    <div className="col">
      <label>{name}: {value} {unit}</label>
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) =>  {
          onChange(Number(e.target.value));
        }}
      />
    </div>
  );
}

function Doser({index, maxFlowRate}) {
  const [flowRate, setFlowRate] = useState(60)

  return (
    <div>
      <h2>Doser {index}</h2>
      <div className="row">
        <Slider name="flow-rate" unit="mL/min" value={flowRate} onChange={setFlowRate} min={0} max={maxFlowRate}></Slider>
      </div>
      <button onClick={() => { 
          client.publish("sensei/doser/on", JSON.stringify({
            doserID: index,
            flowRate: flowRate
          }));
        }}>On</button>
      <button onClick={() => { 
          client.publish("sensei/doser/off", JSON.stringify({
            doserID: index,
          }));
        }}>Off</button>
    </div>
  );
}

function Sensor({unit, value, onCalibrate, onReset}) {
  const [target, setTarget] = useState(0)

  return (
    <>
      <h2>{unit} Sensor: {value}</h2>
      <Slider name="actual" unit={unit} value={target} onChange={setTarget} min={0} max={14} step="0.1"></Slider>
      <button onClick={() => { onCalibrate(target)}}>Calibrate</button>
      <button onClick={() => { onReset()}}>Factory Reset</button>
    </>
  )
}

function NutrientController({status, phDosers}) {
  const [target, setTarget] = useState(7.0);
  const [acceptedError, setAcceptedError] = useState(0);
  const [adjustInterval, setAdjustInterval] = useState(0);
  const [flowRate, setFlowRate] = useState(0);
  const [schedule, setSchedule] = useState({}); // Default value

  const isRunning = () => { return status["nutrientControllerRunning"] }

  const updateSchedule = (doserID) => {
    return (doseAmount) => {
      let newSchedule = schedule;
      newSchedule[doserID] = doseAmount;
      setSchedule(newSchedule);
    }
  }

  const totalAmount = () => {
    let result = 0
    for (let i = 0; i < schedule.length; i++) {
      result += schedule[i]
    }
    return result;
  }

  const scheduleUI = () => {
    const doses = [];
    for (let i = 0; i < status.dosers.length; i++) {
      if (phDosers.has(i)) {
        doses.push(
          <div className="alert" key={i}>
            <Slider2 key={i} name={"doser[" + i + "]"} unit="mL/L" value={0} onChange={(_) => {}} min={0} max={10}></Slider2>
          </div>);
      } else {
        doses.push(<Slider key={i} name={"doser[" + i + "]"} unit="mL/L" onChange={updateSchedule(i)} min={0} max={1} step="0.01"></Slider>);
      }
    }
    return doses;
  };


  return (
    <>
      <h2>Nutrient controller</h2>
      <Slider name="target" unit="EC" onChange={setTarget} min={0} max={4} step="0.1"></Slider>
      <Slider name="accepted error" unit="EC" onChange={setAcceptedError} min={0} max={1} step="0.01"></Slider>
      <Slider name="adjust-interval" unit="sec" onChange={setAdjustInterval} min={0} max={120}></Slider>
      <Slider name="flow-rate" unit="mL/min" onChange={setFlowRate} min={0} max={60}></Slider>
      <h3>Schedule</h3>
      <p>Total of {totalAmount()}mL takes {flowRate == 0 ? "inf" : totalAmount() / flowRate}min to dose</p>
      <div className="schedule-container">
        {scheduleUI()}
      </div>
      <button onClick={() => { 
          if (!isRunning()) {
            let actualSchedule = {};
            for (const doserID in schedule) {
              if (schedule[doserID] > 0) {
                actualSchedule[doserID] = schedule[doserID];
              }
            }

            client.publish("sensei/nutrientController/start", JSON.stringify({
              config: {
                target: Number(target),
                acceptedError: Number(acceptedError),
                adjustmentInterval: Number(adjustInterval),
                flowRate: Number(flowRate),
                schedule: actualSchedule
              }
            }));

          } else {
            client.publish("sensei/nutrientController/stop", "");
          }
        }}>{isRunning() ? "Stop" : "Start"}</button>
    </>
  )
}


function PHController({numDosers, onDoserChange}) {
  const [target, setTarget] = useState(7.0);
  const [acceptedError, setAcceptedError] = useState(0);
  const [flowRate, setFlowRate] = useState(0);
  const [doseAmount, setDoseAmount] = useState(0);
  const [adjustInterval, setAdjustInterval] = useState(0);
  const [isRunning, setIsRunning] = useState(false);

  const none = "none";
  const [pHDownDoser, setpHDownDoser] = useState(none); // Default value
  const [pHUpDoser, setpHUpDoser] = useState(none); // Default value

  useEffect(() => {
    messageHandler("sensei/status", (message) => {
      try {
        const status = JSON.parse(message.toString());
        setIsRunning(status["pHControllerRunning"]);
      } catch (err) {
        console.error(err)
      }
    });
  }, [])

  const handlepHDownDoserChange = (event) => {
    const doserID = Number(event.target.value)
    onDoserChange(pHDownDoser, doserID);
    setpHDownDoser(doserID);
  };

  const handlepHUpDoserChange = (event) => {
    const doserID = Number(event.target.value)
    onDoserChange(pHUpDoser, doserID);
    setpHUpDoser(doserID);
  };

  const generateOptions = (except) => {
    const options = [];
     options.push(<option key={-1} value={-1}>{none}</option>);
    for (let i = 0; i < numDosers; i++) {
      if (i == except)
        continue;
      options.push(<option key={i} value={i}>{i}</option>);
    }
    return options;
  };

  return (
    <>
      <h2>pH controller</h2>
      <Slider name="target" unit="pH" onChange={setTarget} min={0} max={14} step="0.1"></Slider>
      <Slider name="accepter error" unit="pH" onChange={setAcceptedError} min={0} max={1} step="0.01"></Slider>
      <Slider name="adjust-interval" unit="sec" onChange={setAdjustInterval} min={0} max={100}></Slider>
      <Slider name="flow-rate" unit="mL/min" onChange={setFlowRate} min={0} max={60}></Slider>
      <Slider name="dose-amount" unit="mL" onChange={setDoseAmount} min={0} max={10} step="0.1"></Slider>
      <div className="row pm">
        <div className="row">
          <label htmlFor="pHDownDoser">pH Down Doser</label>
          <select         
            id="pHDownDoser"
            name="pHDownDoser"
            value={pHDownDoser}
            onChange={handlepHDownDoserChange}
          >
            {generateOptions(pHUpDoser)}
          </select>
        </div>

        <div className="row">
          <label htmlFor="pHUpDoser">pH Up Doser</label>
          <select
            id="pHUpDoser"
            name="pHUpDoser"
            value={pHUpDoser}
            onChange={handlepHUpDoserChange}
          >
            {generateOptions(pHDownDoser)}
          </select>
        </div>
        </div>
      <button onClick={() => { 
          if (!isRunning) {
            client.publish("sensei/pHController/start", JSON.stringify({
              config: {
                target: Number(target),
                acceptedError: Number(acceptedError),
                adjustInterval: Number(adjustInterval),
                flowRate: Number(flowRate),
                doseAmount: Number(doseAmount),
                pHDownDoser: Number(pHDownDoser),
                pHUpDoser: Number(pHUpDoser)
              }
            }));
            setIsRunning(true);
          } else {
            client.publish("sensei/pHController/stop", "");
            setIsRunning(false);
          }
        }}>{isRunning ? "Stop" : "Start"}</button>
    </>
  )
}

function App() {
  const [status, setStatus] = useState(null)
  const [prevUpdate, setPrevUpdate] = useState(0)
  const [now, setNow] = useState(Date.now())
  const [maxParallelDosers, setMaxParallelDosers] = useState(1);
  const [phDosers, setPhDosers] = useState(new Set());

  useEffect(() => {
    messageHandler("sensei/status", (message) => {
      setStatus(JSON.parse(message.toString()));
      setPrevUpdate(Date.now());
    });
  }, [])

  useEffect(() => {
    const interval = setInterval(() => {
      setNow(Date.now());
    }, 100);

    return () => clearInterval(interval);
  }, [])

  const isConnected = () => { return now - prevUpdate < 3001; }
  const onPhDoserChange = (prevID, newID) => {
    if (prevID == newID) {
      return;
    } else {
      let dosers = phDosers;
      dosers.delete(prevID);
      dosers.add(newID);
      setPhDosers(dosers);
    }
  }

  return (
    <>
      <p>Last seen: {now - prevUpdate}ms ago</p>
      {isConnected() ? (
        <div className="col">
          <h1>Sensors</h1>
          <div className="row--wrap">
            <div className="card">
              <Sensor 
                unit="pH" 
                value={status.ph.toFixed(2)} 
                onCalibrate={(target) => {
                  client.publish("sensei/pHSensor/calibrate", JSON.stringify({
                    target: target
                  }))
                }}
                onReset={() => client.publish("sensei/pHSensor/factoryReset", "")}/>
            </div>
            <div className="card">
              <Sensor 
                unit="EC" 
                value={status.ec.toFixed(2)} 
                onCalibrate={(target) => {
                  client.publish("sensei/ecSensor/calibrate", JSON.stringify({
                    target: target
                  }))
                }}
                onReset={() => client.publish("sensei/ecSensor/factoryReset", "")}/>
            </div>
          </div>

          <h1>Controllers</h1>
          <div className="row--wrap">
            <div className="card">
              <PHController status={status} numDosers={status.dosers.length} onDoserChange={onPhDoserChange}></PHController>
            </div>
            <div className="card">
              <NutrientController status={status} numDosers={status.dosers.length} phDosers={phDosers}></NutrientController>
            </div>
          </div>

          <h1>Dosers</h1>
          <div className="center card">
            <div className="pm col">
              <Slider2 name="MaxParallelDosers" unit="" value={maxParallelDosers} onChange={(value) => {
                  setMaxParallelDosers(value);
                  client.publish("sensei/doserManager/setConfig", JSON.stringify(
                    { 
                      config: {
                        maxParallelDosers: value 
                      }
                    }
                  ));

                }} min={1} max={8} step="1"></Slider2>
              <button onClick={() => client.publish("sensei/doserManager/reset", "") }>Stop all</button>
            </div>
            <div className="row--wrap">
            {status.dosers.map((doser, index) => (

              <div key={index}>
              <Doser index={index} maxFlowRate={doser.maxFlowRate}></Doser>
              </div>
            ))}
            </div>
          </div>

        </div>
      ) : (
        <p>Connecting...</p>
      )}
    </>
  )
}

export default App
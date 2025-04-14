import axios from "axios";


const URL_BASE = "http://192.168.1.29:80"

export const getStatus = async () => {
  try {
    const response = await axios.get(URL_BASE + "/status");
    return response.data;
  } catch (error) {
    console.error("Error fetching status:", error);
    throw error;
  }
};

export const dose = async (doserID, flowRate, amount) => {
  try {
    const response = await axios.post(URL_BASE + "/dose", {
            doserID: doserID,
            flowRate: flowRate,
            amount: amount
        }
    );
    return response.data;
  } catch (error) {
    console.error("Error fetching status:", error);
    throw error;
  }
};

export const calibratePH = async (target) => {
  try {
    const response = await axios.post(URL_BASE + "/calibratePh", {
            target: target 
        }
    );
    return response.data;
  } catch (error) {
    console.error("Error fetching status:", error);
    throw error;
  }
}

export const calibrateEC = async (target) => {
  try {
    const response = await axios.post(URL_BASE + "/calibrateEc", {
            target: target 
        }
    );
    return response.data;
  } catch (error) {
    console.error("Error fetching status:", error);
    throw error;
  }
}

export const startPHController = async (config) => {
  try {
    const response = await axios.post(URL_BASE + "/startController", {
            name: "ph",
            config: config 
        }
    );
    return response.data;
  } catch (error) {
    console.error("Error fetching status:", error);
    throw error;
  }
}

export const startNutrientController = async (config) => {
  try {
    const response = await axios.post(URL_BASE + "/startController", {
            name: "nutrient",
            config: config 
        }
    );
    return response.data;
  } catch (error) {
    console.error("Error fetching status:", error);
    throw error;
  }
}

export const stopPHController = async (config) => {
  try {
    const response = await axios.post(URL_BASE + "/stopController", {
            name: "ph"
        }
    );
    return response.data;
  } catch (error) {
    console.error("Error fetching status:", error);
    throw error;
  }
}

export const stopNutrientController = async (config) => {
  try {
    const response = await axios.post(URL_BASE + "/stopController", {
            name: "nutrient"
        }
    );
    return response.data;
  } catch (error) {
    console.error("Error fetching status:", error);
    throw error;
  }
}
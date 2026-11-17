
const API_BASE = "http://localhost:18080";

let selectedTrain = null;
let selectedJourneyDate = "";
let authMode = "login";

// =============================================
// COMMON ELEMENTS
// =============================================

const journeyDate = document.getElementById("journeyDate");
const searchForm = document.getElementById("searchForm");
const searchMessage = document.getElementById("searchMessage");

const authModal = document.getElementById("authModal");
const authForm = document.getElementById("authForm");

const loginBtn = document.getElementById("loginBtn");
const registerBtn = document.getElementById("registerBtn");
const logoutBtn = document.getElementById("logoutBtn");

const myBookingsBtn = document.getElementById("myBookingsBtn");
const myBookingsSection = document.getElementById("myBookings");
const bookingsContainer = document.getElementById("bookingsContainer");
const bookingsMessage = document.getElementById("bookingsMessage");
const refreshBookingsBtn = document.getElementById("refreshBookingsBtn");

const pnrSearchBtn = document.getElementById("pnrSearchBtn");
const pnrSearch = document.getElementById("pnrSearch");
const pnrResult = document.getElementById("pnrResult");

// Authentication elements
const closeAuth = document.getElementById("closeAuth");
const authTitle = document.getElementById("authTitle");
const authSubtitle = document.getElementById("authSubtitle");
const authSubmit = document.getElementById("authSubmit");
const authMessage = document.getElementById("authMessage");
const premiumField = document.getElementById("premiumField");
const premiumCheckbox = document.getElementById("premiumCheckbox");
const authSwitch = document.getElementById("authSwitch");

// Booking modal elements
const bookingModal = document.getElementById("bookingModal");
const bookingClass = document.getElementById("bookingClass");
const bookingFare = document.getElementById("bookingFare");
const closeBookingModal = document.getElementById("closeBookingModal");
const bookingForm = document.getElementById("bookingForm");

const bookingTrainName = document.getElementById("bookingTrainName");
const bookingTrainRoute = document.getElementById("bookingTrainRoute");
const bookingTrainNumber = document.getElementById("bookingTrainNumber");

const journeyDateInput = document.getElementById("bookingJourneyDate");
const bookingSeatNumber = document.getElementById("bookingSeatNumber");
const bookingError = document.getElementById("bookingError");


// =============================================
// DATE HELPERS
// =============================================

function getLocalDate() {
    const now = new Date();

    return new Date(
        now.getTime() - now.getTimezoneOffset() * 60000
    ).toISOString().split("T")[0];
}

const localToday = getLocalDate();

if (journeyDate) {
    journeyDate.min = localToday;
}

if (journeyDateInput) {
    journeyDateInput.min = localToday;
}


// =============================================
// TRAIN IMAGE SELECTION
// =============================================

function getTrainPhotos() {
    const images = Array.from(document.querySelectorAll("img"));

    const trainImages = images
        .filter(function (img) {
            const description = (
                (img.alt || "") + " " +
                (img.src || "")
            ).toLowerCase();

            return (
                description.includes("vande") ||
                description.includes("rajdhani") ||
                description.includes("train") ||
                description.includes("rail")
            );
        })
        .map(function (img) {
            return img.currentSrc || img.src;
        })
        .filter(function (src) {
            return src && !src.startsWith("data:");
        });

    return [...new Set(trainImages)];
}

function getRandomTrainPhoto() {
    const photos = getTrainPhotos();

    if (photos.length > 0) {
        const randomIndex = Math.floor(
            Math.random() * photos.length
        );

        return photos[randomIndex];
    }

    const hero = document.querySelector(".hero-train-image");
    return hero ? (hero.currentSrc || hero.src) : "";
}


// =============================================
// TRAIN SEARCH
// =============================================

searchForm.addEventListener("submit", async function (event) {
    event.preventDefault();

    const source = document.getElementById("source").value.trim();
    const destination = document.getElementById("destination").value.trim();
    const date = journeyDate.value;

    selectedJourneyDate = date;

    searchMessage.replaceChildren();

    if (!source || !destination || !date) {
        searchMessage.textContent = "Please enter all journey details.";
        return;
    }

    if (source.toLowerCase() === destination.toLowerCase()) {
        searchMessage.textContent =
            "Source and destination cannot be the same.";

        searchMessage.scrollIntoView({
            behavior: "smooth",
            block: "start"
        });

        return;
    }

    searchMessage.textContent = "Searching for available trains...";

    const params = new URLSearchParams({
        source: source,
        destination: destination,
        date: date
    });

    try {
        const response = await fetch(
            `${API_BASE}/api/trains/search?${params}`
        );

        const data = await response.json();

        if (!response.ok) {
            throw new Error(data.message || "Train search failed.");
        }

        searchMessage.replaceChildren();

        const heading = document.createElement("h2");
        heading.textContent = "Available Trains";

        const summary = document.createElement("p");
        summary.className = "search-summary";
        summary.textContent = `${source} → ${destination} | ${date}`;

        searchMessage.append(heading, summary);

        if (!data.trains || data.trains.length === 0) {
            const empty = document.createElement("p");

            empty.className = "empty-search-message";
            empty.textContent = "No matching trains found for this route.";

            searchMessage.appendChild(empty);

            searchMessage.scrollIntoView({
                behavior: "smooth",
                block: "start"
            });

            return;
        }

        data.trains.forEach(function (train) {
            const card = document.createElement("div");
            card.className = "train-card";

            const trainImage = document.createElement("img");

            trainImage.className = "train-card-image";
            trainImage.src = getRandomTrainPhoto();
            trainImage.alt = "Indian passenger train";
            trainImage.loading = "lazy";

            trainImage.onerror = function () {
                this.onerror = null;
                this.style.display = "none";
            };

            const details = document.createElement("div");
            details.className = "train-card-details";

            const name = document.createElement("h3");
            name.textContent = train.trainName;

            const number = document.createElement("p");
            number.className = "train-number";
            number.textContent = `Train Number: ${train.trainNumber}`;

            const route = document.createElement("p");
            route.className = "train-route";
            route.textContent = `${train.source} → ${train.destination}`;

            const fare = document.createElement("p");
            fare.className = "train-fare";
            fare.textContent = `Fare: ₹${train.fare}`;

            const seats = document.createElement("div");
            seats.className = "seat-availability";

            const acSeats = document.createElement("div");
            acSeats.className = "seat-category ac-category";

            const acHeading = document.createElement("h4");
            acHeading.textContent = "AC Class";

            const acDetails = document.createElement("p");
            acDetails.textContent =
                `${train.availableSeatsAC}/${train.totalSeatsAC} Seats Available`;

            acSeats.append(acHeading, acDetails);

            const sleeperSeats = document.createElement("div");
            sleeperSeats.className = "seat-category sleeper-category";

            const sleeperHeading = document.createElement("h4");
            sleeperHeading.textContent = "Sleeper Class";

            const sleeperDetails = document.createElement("p");
            sleeperDetails.textContent =
                `${train.availableSeatsSleeper}/${train.totalSeatsSleeper} Seats Available`;

            sleeperSeats.append(sleeperHeading, sleeperDetails);

            seats.append(acSeats, sleeperSeats);

            const button = document.createElement("button");

            button.type = "button";
            button.className = "book-train-btn";
            button.textContent = "Book Now";

            button.addEventListener("click", function () {
                const token = sessionStorage.getItem("railbookToken");

                if (!token) {
                    alert("Please log in before booking a ticket.");

                    authModal.style.display = "flex";
                    return;
                }

                selectedTrain = train;

                bookingTrainName.textContent = train.trainName;

                bookingTrainRoute.textContent =
                    `${train.source} → ${train.destination}`;

                bookingTrainNumber.textContent =
                    `Train Number: ${train.trainNumber}`;

                bookingForm.reset();
                bookingError.textContent = "";

                updateBookingClassDetails();

                journeyDateInput.min = localToday;
                journeyDateInput.value = selectedJourneyDate || localToday;

                bookingModal.style.setProperty(
                    "display",
                    "flex",
                    "important"
                );
            });

            details.append(name, number, route, fare);
            card.append(trainImage, details, button, seats);

            searchMessage.appendChild(card);
        });

        requestAnimationFrame(function () {
            searchMessage.scrollIntoView({
                behavior: "smooth",
                block: "start"
            });
        });

    } catch (error) {
        console.error("Train search error:", error);

        searchMessage.textContent =
            "Unable to connect to the railway server. " +
            "Please check whether the Crow server is running.";

        searchMessage.scrollIntoView({
            behavior: "smooth",
            block: "start"
        });
    }
});


// =============================================
// LOGIN AND REGISTRATION
// =============================================

function openAuth(mode) {
    authMode = mode;

    authModal.style.display = "flex";

    authForm.reset();
    authMessage.textContent = "";
    authMessage.style.color = "";

    if (mode === "register") {
        authTitle.textContent = "Create Your Account";
        authSubtitle.textContent = "Join YatraRail today.";
        authSubmit.textContent = "Register";

        premiumField.hidden = false;

        authSwitch.innerHTML =
            'Already have an account? <a href="#" id="switchAuth">Login here</a>';

    } else {
        authTitle.textContent = "Login to YatraRail";
        authSubtitle.textContent = "Welcome back!";
        authSubmit.textContent = "Login";

        premiumField.hidden = true;

        authSwitch.innerHTML =
            'Don\'t have an account? <a href="#" id="switchAuth">Register here</a>';
    }

    const switchLink = document.getElementById("switchAuth");

    switchLink.addEventListener("click", function (event) {
        event.preventDefault();

        openAuth(authMode === "login" ? "register" : "login");
    });
}

loginBtn.addEventListener("click", function () {
    openAuth("login");
});

registerBtn.addEventListener("click", function () {
    openAuth("register");
});

closeAuth.addEventListener("click", function () {
    authModal.style.display = "none";
});

authModal.addEventListener("click", function (event) {
    if (event.target === authModal) {
        authModal.style.display = "none";
    }
});


// =============================================
// SUBMIT LOGIN / REGISTRATION
// =============================================

authForm.addEventListener("submit", async function (event) {
    event.preventDefault();

    const username = document.getElementById("authUsername").value.trim();
    const password = document.getElementById("authPassword").value;

    if (!username || !password) {
        authMessage.textContent = "Please fill in all fields.";
        return;
    }

    const isRegister = authMode === "register";

    const endpoint = isRegister
        ? "/api/auth/register"
        : "/api/auth/login";

    const requestBody = {
        username: username,
        password: password
    };

    if (isRegister) {
        requestBody.premium = premiumCheckbox.checked;
    }

    authSubmit.disabled = true;
    authSubmit.textContent = "Please wait...";
    authMessage.textContent = "";

    try {
        const response = await fetch(`${API_BASE}${endpoint}`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify(requestBody)
        });

        const data = await response.json();

        if (!response.ok || !data.success) {
            throw new Error(data.message || "Authentication failed.");
        }

        authMessage.style.color = "green";
        authMessage.textContent = data.message;

        if (isRegister) {
            authMessage.textContent =
                `${data.message} Your role: ${data.role}.`;

            setTimeout(function () {
                openAuth("login");
                document.getElementById("authUsername").value = username;
            }, 1500);

        } else {
            sessionStorage.setItem("railbookToken", data.token);
            sessionStorage.setItem("railbookUsername", data.username);
            sessionStorage.setItem("railbookRole", data.role);

            updateNavbar();

            authMessage.textContent =
                `Welcome, ${data.username}! Role: ${data.role}.`;

            setTimeout(function () {
                authModal.style.display = "none";
            }, 1800);
        }

    } catch (error) {
        console.error("Authentication error:", error);

        authMessage.style.color = "red";
        authMessage.textContent = error.message;

    } finally {
        authSubmit.disabled = false;
        authSubmit.textContent =
            authMode === "register" ? "Register" : "Login";
    }
});


// =============================================
// NAVBAR LOGIN STATE
// =============================================

function updateNavbar() {
    const token = sessionStorage.getItem("railbookToken");

    loginBtn.style.display = token ? "none" : "inline-block";
    registerBtn.style.display = token ? "none" : "inline-block";
    logoutBtn.style.display = token ? "inline-block" : "none";

    myBookingsBtn.style.display = token ? "inline-block" : "none";

    if (!token) {
        myBookingsSection.style.display = "none";
    }
}


// =============================================
// BOOKING CLASS FARE UPDATE
// =============================================

function updateBookingClassDetails() {
    if (!selectedTrain) return;

    const selectedClass = bookingClass.value;

    const availabilityHeading =
        document.getElementById("availabilityHeading");

    const availabilityCount =
        document.getElementById("availabilityCount");

    let availableSeats;
    let totalSeats;
    let maxSeats;

    if (selectedClass === "AC") {
        availableSeats = selectedTrain.availableSeatsAC;
        totalSeats = selectedTrain.totalSeatsAC;
        maxSeats = 40;

        availabilityHeading.textContent = "AC Availability";

    } else {
        availableSeats = selectedTrain.availableSeatsSleeper;
        totalSeats = selectedTrain.totalSeatsSleeper;
        maxSeats = 100;

        availabilityHeading.textContent = "Sleeper Availability";
    }

    availabilityCount.textContent =
        `${availableSeats} / ${totalSeats} seats available`;

    bookingSeatNumber.min = 1;
    bookingSeatNumber.max = maxSeats;
    bookingSeatNumber.placeholder =
        `Enter seat number (1-${maxSeats})`;

    const baseFare = Number(selectedTrain.fare);

    const finalFare = selectedClass === "AC"
        ? baseFare * 1.8
        : baseFare;

    bookingFare.textContent = "₹" + finalFare.toFixed(2);
}

bookingClass.addEventListener("change", updateBookingClassDetails);


// =============================================
// MY BOOKINGS
// =============================================

myBookingsBtn.addEventListener("click", function () {
    myBookingsSection.style.display = "block";

    myBookingsSection.scrollIntoView({
        behavior: "smooth",
        block: "start"
    });

    loadBookings();
});


function addTicketDetails(card, label, value) {
    const paragraph = document.createElement("p");

    paragraph.className = "booking-detail";
    paragraph.textContent = `${label}: ${value}`;

    card.appendChild(paragraph);
}


function createBookingCard(booking) {
    const card = document.createElement("div");

    card.className = "booking-card";

    const heading = document.createElement("h3");
    heading.textContent = booking.trainName;

    card.appendChild(heading);

    addTicketDetails(card, "PNR", booking.pnr);
    addTicketDetails(card, "Train Number", booking.trainNumber);

    addTicketDetails(
        card,
        "Route",
        `${booking.source} → ${booking.destination}`
    );

    addTicketDetails(card, "Journey Date", booking.journeyDate);
    addTicketDetails(card, "Seat Number", booking.seatNumber);
    addTicketDetails(card, "Class", booking.classType);
    addTicketDetails(card, "Fare", `₹${booking.fare}`);

    const status = document.createElement("p");

    status.className = "booking-status";

    const isCancelled =
        String(booking.status).toLowerCase() === "cancelled";

    status.textContent = isCancelled
        ? "Cancelled"
        : "Confirmed";

    status.classList.add(
        isCancelled ? "cancelled" : "confirmed"
    );

    card.appendChild(status);

    if (!isCancelled) {
        const cancelButton = document.createElement("button");

        cancelButton.type = "button";
        cancelButton.textContent = "Cancel Ticket";
        cancelButton.className = "cancel-ticket-btn";

        cancelButton.addEventListener("click", function () {
            cancelTicket(booking.pnr);
        });

        card.appendChild(cancelButton);
    }

    return card;
}


// =============================================
// LOAD BOOKINGS
// =============================================

async function loadBookings() {
    const token = sessionStorage.getItem("railbookToken");

    if (!token) {
        bookingsMessage.textContent = "Please log in first.";
        return;
    }

    bookingsMessage.textContent = "Loading your bookings...";
    bookingsContainer.replaceChildren();

    try {
        const response = await fetch(`${API_BASE}/api/bookings`, {
            method: "GET",
            headers: {
                "Authorization": "Bearer " + token
            }
        });

        const data = await response.json();

        if (!response.ok || !data.success) {
            throw new Error(data.message || "Unable to load bookings.");
        }

        bookingsMessage.textContent = "";

        if (!data.bookings || data.bookings.length === 0) {
            bookingsMessage.textContent = "You have no bookings yet.";
            return;
        }

        data.bookings.forEach(function (booking) {
            const card = createBookingCard(booking);

            bookingsContainer.appendChild(card);
        });

    } catch (error) {
        console.error("Load bookings error:", error);

        bookingsMessage.textContent = error.message;
    }
}


// =============================================
// CANCEL TICKET
// =============================================

async function cancelTicket(pnr) {
    const confirmed = confirm(
        `Are you sure you want to cancel ticket PNR ${pnr}?`
    );

    if (!confirmed) return;

    const token = sessionStorage.getItem("railbookToken");

    try {
        const response = await fetch(
            `${API_BASE}/api/bookings/cancel`,
            {
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                    "Authorization": "Bearer " + token
                },
                body: JSON.stringify({
                    pnr: Number(pnr)
                })
            }
        );

        const data = await response.json();

        if (!response.ok || !data.success) {
            throw new Error(
                data.message || "Ticket cancellation failed."
            );
        }

        alert(data.message);

        await loadBookings();

        if (selectedJourneyDate) {
            searchForm.requestSubmit();
        }

    } catch (error) {
        console.error("Cancellation error:", error);

        alert(error.message);
    }
}


// Refresh bookings
refreshBookingsBtn.addEventListener("click", function () {
    loadBookings();
});


// =============================================
// PNR SEARCH
// =============================================

pnrSearchBtn.addEventListener("click", async function () {
    const pnr = pnrSearch.value.trim();

    if (
        !pnr ||
        !Number.isInteger(Number(pnr)) ||
        Number(pnr) <= 0
    ) {
        pnrResult.textContent = "Please enter a valid PNR number.";
        return;
    }

    const token = sessionStorage.getItem("railbookToken");

    if (!token) {
        pnrResult.textContent = "Please log in first.";
        return;
    }

    pnrResult.textContent = "Searching for ticket...";

    try {
        const response = await fetch(
            `${API_BASE}/api/bookings/pnr?pnr=${encodeURIComponent(pnr)}`,
            {
                method: "GET",
                headers: {
                    "Authorization": "Bearer " + token
                }
            }
        );

        const data = await response.json();

        if (!response.ok || !data.success) {
            throw new Error(data.message || "PNR not found.");
        }

        pnrResult.replaceChildren();

        const heading = document.createElement("h3");
        heading.textContent = "PNR Details";

        const card = createBookingCard(data);

        pnrResult.append(heading, card);

    } catch (error) {
        console.error("PNR search error:", error);

        pnrResult.textContent = error.message;
    }
});


// =============================================
// BOOKING MODAL
// =============================================

closeBookingModal.addEventListener("click", function () {
    bookingModal.style.display = "none";
});

bookingModal.addEventListener("click", function (event) {
    if (event.target === bookingModal) {
        bookingModal.style.display = "none";
    }
});


// =============================================
// CONFIRM BOOKING
// =============================================

bookingForm.addEventListener("submit", async function (event) {
    event.preventDefault();

    const token = sessionStorage.getItem("railbookToken");

    if (!token || !selectedTrain) {
        bookingError.textContent =
            "Please log in and select a train.";

        return;
    }

    const seatNumber = Number(bookingSeatNumber.value);
    const selectedDate = journeyDateInput.value;
    const classType = bookingClass.value;

    const maxSeats = classType === "AC" ? 40 : 100;

    if (
        !Number.isInteger(seatNumber) ||
        seatNumber < 1 ||
        seatNumber > maxSeats
    ) {
        bookingError.textContent =
            `Enter a valid ${classType} seat number between 1 and ${maxSeats}.`;

        return;
    }

    if (!selectedDate) {
        bookingError.textContent = "Please select a journey date.";
        return;
    }

    const confirmButton =
        document.getElementById("confirmBookingBtn");

    confirmButton.disabled = true;
    confirmButton.textContent = "Processing...";
    bookingError.textContent = "";

    try {
        const response = await fetch(`${API_BASE}/api/bookings`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
                "Authorization": "Bearer " + token
            },
            body: JSON.stringify({
                trainNumber: selectedTrain.trainNumber,
                seatNumber: seatNumber,
                journeyDate: selectedDate,
                classType: classType
            })
        });

        const data = await response.json();

        if (!response.ok || !data.success) {
            throw new Error(data.message || "Booking failed.");
        }

        alert(
            "TICKET BOOKED SUCCESSFULLY!\n\n" +
            "PNR: " + data.pnr + "\n" +
            "Train: " + data.trainName + "\n" +
            "Train Number: " + data.trainNumber + "\n" +
            "Route: " + data.source + " → " + data.destination + "\n" +
            "Journey Date: " + data.journeyDate + "\n" +
            "Seat Number: " + data.seatNumber + "\n" +
            "Class: " + data.classType + "\n" +
            "Fare: ₹" + data.fare
        );

        bookingModal.style.display = "none";
        bookingForm.reset();

        searchForm.requestSubmit();

        if (myBookingsSection.style.display !== "none") {
            await loadBookings();
        }

    } catch (error) {
        console.error("Booking error:", error);

        bookingError.textContent = error.message;

    } finally {
        confirmButton.disabled = false;
        confirmButton.textContent = "Confirm Booking";
    }
});


// =============================================
// LOGOUT
// =============================================

logoutBtn.addEventListener("click", async function () {
    const token = sessionStorage.getItem("railbookToken");

    try {
        if (token) {
            await fetch(`${API_BASE}/api/auth/logout`, {
                method: "POST",
                headers: {
                    "Authorization": "Bearer " + token
                }
            });
        }

    } catch (error) {
        console.error("Logout request failed:", error);

    } finally {
        sessionStorage.removeItem("railbookToken");
        sessionStorage.removeItem("railbookUsername");
        sessionStorage.removeItem("railbookRole");

        updateNavbar();

        alert("Logged out successfully!");
    }
});


// =============================================
// INITIAL PAGE SETUP
// =============================================

updateNavbar();

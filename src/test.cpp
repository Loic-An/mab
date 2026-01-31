#include "test.hpp"

// SIGINT signal flag
volatile sig_atomic_t Test::should_exit = 0;

// Signal handler function
void signal_handler(int signal)
{
    (void)signal; // Unused parameter
    if (Test::should_exit)
        std::exit(0);
    Test::should_exit = 1;
    printf("\n\nArrêt du programme...\n");
}

int getch()
{
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

Test::Test(int argc, char **argv) : pca9685(nullptr)
{
    signal(SIGINT, signal_handler);
    if (argc == 2)
    {
        std::string arg_scenario = argv[1];
        size_t i = 0;
        while (runnable_scenario[i] != "" && runnable_scenario[i] != arg_scenario)
            i++;
        scenario = runnable_scenario[i] != "" ? static_cast<ScenarioType>(i) : SCENARIO_UNKNOWN;
    }
    else
        scenario = SCENARIO_UNKNOWN;
    if (scenario == SCENARIO_UNKNOWN)
    {
        printf("Scénario inconnu ou non spécifié. Utilisez l'un des suivants : < ");
        for (const auto &name : runnable_scenario)
        {
            if (name != "")
                printf("%s ", name.c_str());
        }
        printf(">\n");
    }
}

Test::~Test()
{
    if (pca9685)
    {
        pca9685->reset();
        delete pca9685;
    }
}

int Test::scenario_calibrage()
{
    // Implémentation du scénario de calibrage
    printf("Exécution du scénario CALIBRAGE...\n");
    PCA9685 pca(0x40);
    if (!pca.init())
    {
        printf("Erreur d'initialisation du PCA9685\n");
        return 1;
    }

    int motor_selected = 0;
    bool running = true;

    printf("\e[2J\e[H"); // Efface l'écran
    printf("=== CONTROLE MANUEL DES MOTEURS ===\n");
    printf("Utilisez les fleches GAUCHE/DROITE pour choisir le moteur\n");
    printf("Utilisez les fleches HAUT/BAS pour faire tourner\n");
    printf("Appuyez sur 'q' pour quitter\n\n");

    while (running && !should_exit)
    {
        printf("\rMoteur actif : [%d] (Canaux PCA %d & %d)          ",
               motor_selected, motor_selected * 2, motor_selected * 2 + 1);
        fflush(stdout);

        int c = getch();

        if (c == 'q')
        {
            running = false;
        }
        else if (c == 27)
        {            // Séquence d'échappement pour les flèches
            getch(); // ignore [
            switch (getch())
            {
            case 'A': // HAUT
                pca.set_pwm(motor_selected * 2, PCA9685::MAX_PWM);
                pca.set_pwm(motor_selected * 2 + 1, 0);
                usleep(100000); // Tourne pendant 100ms
                break;
            case 'B': // BAS
                pca.set_pwm(motor_selected * 2, 0);
                pca.set_pwm(motor_selected * 2 + 1, PCA9685::MAX_PWM);
                usleep(100000);
                break;
            case 'C': // DROITE
                motor_selected = (motor_selected + 1) % 4;
                break;
            case 'D': // GAUCHE
                motor_selected = (motor_selected + 3) % 4;
                break;
            }
            // Arrêt automatique après le mouvement
            pca.set_pwm(motor_selected * 2, 0);
            pca.set_pwm(motor_selected * 2 + 1, 0);
        }
    }

    // Sécurité : tout éteindre avant de quitter
    for (int i = 0; i < 16; i++)
        pca.set_pwm(i, 0);
    printf("\nFin du contrôle manuel.\n");
    return 0;

    return 0;
}
int Test::scenario_matrix()
{
    // Implémentation du scénario matrix
    printf("Exécution du scénario MATRIX...\n");

    freenect_context *ctx;
    if (freenect_init(&ctx, NULL) < 0)
    {
        printf("ERREUR : freenect_init a échoué. Problème de libfreenect.\n");
    }
    else
    {
        int num_devices = freenect_num_devices(ctx);
        printf("DEBUG : Nombre de Kinect détectées par le driver : %d\n", num_devices);
        freenect_shutdown(ctx);
    }
    printf("\e[1;1H\e[2J");

    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;

    printf("Initialisation Kinect...\n");
    // Un premier appel pour "réveiller" le hardware
    freenect_sync_get_depth((void **)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);
    usleep(500000); // Pause

    while (!should_exit)
    {
        // On récupère la profondeur
        int ret = freenect_sync_get_depth((void **)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_11BIT);
        if (ret != 0)
        {
            printf("Erreur de lecture...\n");
            usleep(500000);
            continue;
        }

        printf("\e[H");

        for (int y = 0; y < 320; y += stY)
        {
            for (int x = 0; x < 640; x += stX)
            {
                uint16_t d = depth_buffer[y * 640 + x];

                // 2047 est le code "sans donnée" ou "trop proche/loin".
                if (d >= 2047 || d == 0)
                    printf("    ");
                else if (d < 600) // TRÈS PROCHE
                    printf(" ###");
                else if (d < 950) // PROCHE
                    printf(" ooo");
                else if (d < 1300) // MILIEU
                    printf(" ---");
                else // LOIN
                    printf("  . ");
            }
            printf("\n");
        }
        // ~30 fps
        usleep(33000);
    }
    freenect_sync_stop();
    return 0;
}

int Test::scenario_vl53l0x()
{
    VL53L0X dev;
    if (!dev.init())
    {
        fprintf(stderr, "Erreur initialisation VL53L0X\n");
        return EXIT_FAILURE;
    }
    printf("VL53L0X initialisé avec succès\n");
    printf("Mesure de distance...\n");
    while (!should_exit)
    {
        uint16_t distancemoyenne = 0;
        for (int i = 0; i < 30; i++)
        {
            uint16_t distance = dev.readRangeSingleMillimeters();
            distancemoyenne += distance;
            if (dev.timeoutOccurred())
            {
                printf("Timeout !\n");
            }
        }
        printf("Distance : %u mm\n", distancemoyenne / 30);
        usleep(1000);
    }
    return EXIT_SUCCESS;
}

int Test::scenario_tiltkinect()
{
    freenect_context *ctx;
    freenect_device *dev;

    if (freenect_init(&ctx, NULL) < 0)
        return 1;

    // Ouvre le device 0
    if (freenect_open_device(ctx, &dev, 0) < 0)
    {
        printf("Impossible d'ouvrir la Kinect\n");
        return 1;
    }

    printf("Test du moteur : Inclinaison à +15°\n");
    freenect_set_tilt_degs(dev, 15);
    sleep(2);

    printf("Inclinaison à -15°\n");
    freenect_set_tilt_degs(dev, -15);
    sleep(2);

    printf("Retour à 0°\n");
    freenect_set_tilt_degs(dev, 0);

    freenect_close_device(dev);
    freenect_shutdown(ctx);
    return 0;
}
int Test::scenario_kinect_sync()
{
    uint16_t *depth_buffer = NULL;
    uint32_t timestamp;

    // Coordonnées du pixel central (ecran de 640x480)
    int x = 320;
    int y = 240;
    int index = y * 640 + x;

    printf("Lecture de la Kinect (Mode Synchrone)...\n");

    while (!should_exit)
    {
        // Récupération synchrone de la frame de profondeur
        // Cette fonction attend qu'une nouvelle frame soit disponible
        int ret = freenect_sync_get_depth((void **)&depth_buffer, &timestamp, 0, FREENECT_DEPTH_MM);
        if (ret != 0)
        {
            printf("Erreur : Impossible de récupérer les données (Kinect déconnectée ?)\n");
            break;
        }

        // Accès à la valeur brute du pixel central
        uint16_t depth = depth_buffer[index];

        if (depth >= FREENECT_DEPTH_MM_MAX_VALUE || depth == FREENECT_DEPTH_MM_NO_VALUE)
            printf("Pixel [%d, %d] : Hors de portée / Trop proche\n", x, y);
        else
            printf("Pixel [%d, %d] : Distance brute = %d mm\n", x, y, depth);

        // Optionnel : un petit délai pour ne pas saturer le processeur
        usleep(100000);
    }

    return 0;
}

// Callback appelé à chaque nouvelle image de profondeur (mode asynchrone)
void depth_cb(freenect_device *dev, void *v_depth, uint32_t timestamp)
{
    (void)dev; // Unused parameter
    // Exemple : lire la distance du pixel central (320x240)
    printf("[%d]Profondeur au centre : %d mm\n", timestamp, ((uint16_t *)v_depth)[320 + 240 * 640]);
}

int Test::scenario_kinect_async()
{
    freenect_context *ctx;
    freenect_device *dev;
    // 1. Initialisation
    if (freenect_init(&ctx, NULL) < 0)
        return 1;

    // 2. Ouverture du premier périphérique trouvé
    if (freenect_open_device(ctx, &dev, 0) < 0)
        return 1;

    // 3. Configuration de la profondeur
    freenect_set_depth_callback(dev, depth_cb);
    freenect_set_depth_mode(dev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_MM));

    // 4. Démarrage du flux
    freenect_start_depth(dev);

    printf("Lecture des données... Appuyez sur Ctrl+C pour arrêter.\n");

    // 5. Boucle principale pour traiter les événements USB
    while (freenect_process_events(ctx) >= 0)
    {
        // Le programme tourne ici et appelle depth_cb automatiquement
    }

    freenect_stop_depth(dev);
    freenect_close_device(dev);
    freenect_shutdown(ctx);

    return 0;
}

int Test::scenario_pca9685(uint8_t a)
{
    pca9685 = new PCA9685(a);

    printf("Recherche du PCA9685 sur %02x...\n", a);
    if (!pca9685->init())
    {
        printf("ERREUR: PCA9685 non trouvé\n");
        return 1;
    }
    // test du pca9685

    while (!should_exit)
    {
        pca9685->set_pwm(1, 2500); // 100% duty cycle
        usleep(1000000);           // wait 1000ms
        pca9685->set_pwm(1, 0);    // turn off
        usleep(1000000);           // wait 1000ms
    }
    // reset and deletion done in destructor, so that you can do other scenarios
    return 0;
}

int Test::scenario_mixed()
{
    int status = 0;
    printf("===== Test Freenect Sync =====\n");
    status |= this->scenario_kinect_sync();
    printf("status=%d\n", status);
    printf("\n===== Test Freenect Async =====\n");
    status |= this->scenario_kinect_async();
    printf("status=%d\n", status);
    printf("\n===== Test VL53L0X =====\n");
    status |= this->scenario_vl53l0x();
    printf("status=%d\n", status);
    return status;
}

int Test::run()
{
    switch (scenario)
    {
    case SCENARIO_CALIBRAGE:
        return this->scenario_calibrage();
    case SCENARIO_MATRIX:
        return this->scenario_matrix();
    case SCENARIO_VL53L0X:
        return this->scenario_vl53l0x();
    case SCENARIO_PCA9685:
        return this->scenario_pca9685();
    case SCENARIO_TILTKINECT:
        return this->scenario_tiltkinect();
    case SCENARIO_KINECT_SYNC:
        return this->scenario_kinect_sync();
    case SCENARIO_KINECT_ASYNC:
        return this->scenario_kinect_async();
    case SCENARIO_MIXED:
        return this->scenario_mixed();
    default:
        return 1;
    }
}
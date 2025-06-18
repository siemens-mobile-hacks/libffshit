pipeline {
    agent any
    stages {
        stage('Build') { 
            steps {
                sh 'rm -rf build'
                sh 'cmake -B build -S .' 
                sh 'cmake --build build'
            }
        }
    }
}

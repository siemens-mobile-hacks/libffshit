pipeline {
    agent none
    stages {
        stage('Build ubuntu') {
            agent {
                node {
                    label 'ubuntu'
                }
            }
            steps {
                sh 'rm -rf build'
                sh 'cmake -B build -S .' 
                sh 'cmake --build build'
            }
        }
    }
}
